#include "gameplay/items/InventorySave.h"

#include <array>
#include <bit>
#include <cstring>
#include <limits>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
#endif


namespace ve
{
    namespace
    {
        using Bytes = std::vector<std::uint8_t>;

        void put16( Bytes& bytes, std::uint16_t v ) {
            bytes.push_back(static_cast<std::uint8_t>(v));
            bytes.push_back(static_cast<std::uint8_t>(v >> 8));
        }

        void put32( Bytes& bytes, std::uint32_t v ) {
            for (int i = 0; i < 4; ++i)
                bytes.push_back(static_cast<std::uint8_t>(v >> (i * 8)));
        }

        std::uint16_t get16( const Bytes& bytes, std::size_t& offset ) {
            if (offset + 2 > bytes.size())
                throw std::runtime_error("Truncated inventory file");
            const std::uint16_t value = static_cast<std::uint16_t>(bytes[offset]) |
                static_cast<std::uint16_t>(bytes[offset + 1] << 8);
            offset += 2;
            return value;
        }

        std::uint32_t get32( const Bytes& bytes, std::size_t& offset ) {
            const std::uint32_t lo = get16(bytes, offset);
            const std::uint32_t hi = get16(bytes, offset);
            return lo | (hi << 16);
        }

        void put64( Bytes& bytes, std::uint64_t value ) {
            put32(bytes, static_cast<std::uint32_t>(value));
            put32(bytes, static_cast<std::uint32_t>(value >> 32));
        }

        std::uint64_t get64( const Bytes& bytes, std::size_t& offset ) {
            const std::uint64_t low = get32(bytes, offset);
            const std::uint64_t high = get32(bytes, offset);
            return low | (high << 32);
        }

        void putFloat( Bytes& bytes, float value ) { put32(bytes, std::bit_cast<std::uint32_t>(value)); }
        float getFloat( const Bytes& bytes, std::size_t& offset ) {
            return std::bit_cast<float>(get32(bytes, offset));
        }

        void putString( Bytes& bytes, const std::string& value );

        void putStack( Bytes& bytes, const ItemStack& stack, const ItemRegistry& registry ) {
            stack.validate(registry);
            put16(bytes, static_cast<std::uint16_t>(stack.id));
            put16(bytes, stack.count);
            put16(bytes, static_cast<std::uint16_t>(stack.metadata.size()));
            for (const auto& [key, value] : stack.metadata) {
                putString(bytes, key);
                putString(bytes, value);
            }
        }

        void putString( Bytes& bytes, const std::string& value ) {
            if (value.size() > 256)
                throw std::invalid_argument("Inventory metadata string too large");
            put16(bytes, static_cast<std::uint16_t>(value.size()));
            bytes.insert(bytes.end(), value.begin(), value.end());
        }

        std::string getString( const Bytes& bytes, std::size_t& offset ) {
            const std::size_t length = get16(bytes, offset);
            if (length > 256 || offset + length > bytes.size())
                throw std::runtime_error("Invalid inventory metadata string");
            std::string result(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                               bytes.begin() + static_cast<std::ptrdiff_t>(offset + length));
            offset += length;
            return result;
        }

        std::uint32_t checksum( const Bytes& bytes, std::size_t length ) {
            std::uint32_t result = 2166136261u;
            for (std::size_t i = 0; i < length; ++i) {
                result ^= bytes[i];
                result *= 16777619u;
            }
            return result;
        }
    }


    void InventorySave::write( const std::filesystem::path& path, const Inventory& inventory,
                               const WorldItems* world ) {
        Bytes bytes{ 'V', 'E', 'I', '3' };
        put16(bytes, 2);
        put16(bytes, static_cast<std::uint16_t>(inventory.size()));
        put16(bytes, static_cast<std::uint16_t>(inventory.hotbarSize()));
        put16(bytes, static_cast<std::uint16_t>(inventory.selectedHotbar()));

        for (std::size_t i = 0; i < inventory.size(); ++i) {
            const auto& stack = inventory.at(i);
            if (!stack) {
                put16(bytes, 0);
                continue;
            }
            putStack(bytes, *stack, inventory.registry());
        }
        const std::size_t dropCount = world != nullptr ? world->all().size() : 0;
        if (dropCount > 4096)
            throw std::invalid_argument("Too many world item drops");
        put16(bytes, static_cast<std::uint16_t>(dropCount));
        if (world != nullptr) {
            for (const WorldItem& drop : world->all()) {
                put64(bytes, drop.id);
                putStack(bytes, drop.stack, inventory.registry());
                putFloat(bytes, drop.position.x);
                putFloat(bytes, drop.position.y);
                putFloat(bytes, drop.position.z);
            }
        }
        put32(bytes, checksum(bytes, bytes.size()));

        if (!path.parent_path().empty())
            std::filesystem::create_directories(path.parent_path());
        const std::filesystem::path temporary = path.string() + ".tmp";
        {
            std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
            if (!stream)
                throw std::runtime_error("Unable to create temporary inventory file");
            stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            stream.flush();
            if (!stream)
                throw std::runtime_error("Inventory file write failed");
        }
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Unable to replace inventory save file");
#else
        std::filesystem::rename(temporary, path);
#endif
    }

    bool InventorySave::read( const std::filesystem::path& path, Inventory& inventory,
                              WorldItems* world ) {
        if (!std::filesystem::exists(path))
            return false;
        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        if (!stream || stream.tellg() < 16 || stream.tellg() > 25165824)
            throw std::runtime_error("Invalid inventory save size");
        const auto size = static_cast<std::size_t>(stream.tellg());
        stream.seekg(0);
        Bytes bytes(size);
        stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
        if (!stream)
            throw std::runtime_error("Inventory save read failed");
        std::size_t checksumOffset = size - 4;
        if (bytes[0] != 'V' || bytes[1] != 'E' || bytes[2] != 'I' || bytes[3] != '3' ||
            get32(bytes, checksumOffset) != checksum(bytes, size - 4))
            throw std::runtime_error("Inventory signature or checksum mismatch");
        std::size_t offset = 4;
        if (get16(bytes, offset) != 2 || get16(bytes, offset) != inventory.size() ||
            get16(bytes, offset) != inventory.hotbarSize())
            throw std::runtime_error("Inventory schema mismatch");
        const std::size_t selected = get16(bytes, offset);
        if (selected >= inventory.hotbarSize())
            throw std::runtime_error("Invalid selected hotbar index");
        Inventory next(inventory.registry(), inventory.size(), inventory.hotbarSize());
        next.selectHotbar(selected);
        for (std::size_t i = 0; i < next.size(); ++i) {
            const ItemId id = static_cast<ItemId>(get16(bytes, offset));
            if (id == ItemId::None)
                continue;
            ItemStack stack{ id, get16(bytes, offset), {} };
            const std::uint16_t attributes = get16(bytes, offset);
            if (attributes > 16)
                throw std::runtime_error("Too many inventory metadata fields");
            for (std::uint16_t j = 0; j < attributes; ++j) {
                const std::string key = getString(bytes, offset);
                const std::string value = getString(bytes, offset);
                const bool inserted = stack.metadata.emplace(key, value).second;
                if (!inserted)
                    throw std::runtime_error("Duplicate inventory metadata key");
            }
            next.set(i, std::move(stack));
        }
        const std::size_t dropCount = get16(bytes, offset);
        if (dropCount > 4096)
            throw std::runtime_error("Too many saved world drops");
        std::vector<WorldItem> drops;
        drops.reserve(dropCount);
        for (std::size_t i = 0; i < dropCount; ++i) {
            WorldItem drop{};
            drop.id = get64(bytes, offset);
            drop.stack.id = static_cast<ItemId>(get16(bytes, offset));
            drop.stack.count = get16(bytes, offset);
            const std::size_t attributes = get16(bytes, offset);
            if (attributes > 16)
                throw std::runtime_error("Too many world item metadata fields");
            for (std::size_t j = 0; j < attributes; ++j) {
                const std::string key = getString(bytes, offset);
                const std::string value = getString(bytes, offset);
                if (!drop.stack.metadata.emplace(key, value).second)
                    throw std::runtime_error("Duplicate world item metadata key");
            }
            drop.stack.validate(inventory.registry());
            drop.position = Vec3{ getFloat(bytes, offset), getFloat(bytes, offset), getFloat(bytes, offset) };
            drops.push_back(std::move(drop));
        }
        if (offset != size - 4)
            throw std::runtime_error("Unexpected trailing inventory data");
        if (world != nullptr) {
            WorldItems validated(inventory.registry());
            validated.restore(drops);
            world->restore(std::move(drops));
        } else if (dropCount != 0) {
            throw std::runtime_error("Saved world drops require a WorldItems destination");
        }
        inventory = std::move(next);
        return true;
    }
}
