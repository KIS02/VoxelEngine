#include "gameplay/save/PlayerSave.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
#endif


namespace ve
{
    namespace
    {
        using Bytes = std::vector<std::uint8_t>;

        void put32( Bytes& data, std::uint32_t value ) {
            for (int i = 0; i < 4; ++i)
                data.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
        }

        std::uint32_t get32( const Bytes& data, std::size_t& offset ) {
            if (data.size() - offset < 4)
                throw std::runtime_error("Player save truncated");
            std::uint32_t result = 0;
            for (int i = 0; i < 4; ++i)
                result |= static_cast<std::uint32_t>(data[offset++]) << (i * 8);
            return result;
        }

        void putFloat( Bytes& data, float value ) {
            put32(data, std::bit_cast<std::uint32_t>(value));
        }

        float getFloat( const Bytes& data, std::size_t& offset ) {
            return std::bit_cast<float>(get32(data, offset));
        }

        void putVec3( Bytes& data, const Vec3& value ) {
            putFloat(data, value.x);
            putFloat(data, value.y);
            putFloat(data, value.z);
        }

        Vec3 getVec3( const Bytes& data, std::size_t& offset ) {
            return Vec3{ getFloat(data, offset), getFloat(data, offset), getFloat(data, offset) };
        }

        std::uint32_t checksum( const Bytes& data, std::size_t size ) {
            std::uint32_t result = 2166136261u;
            for (std::size_t i = 0; i < size; ++i) {
                result ^= data[i];
                result *= 16777619u;
            }
            return result;
        }

        void validatePlayer( const PlayerState& player ) {
            player.stats.validate();
            if (!player.spawned || !PlayerState::finitePosition(player.position) ||
                !PlayerState::finitePosition(player.spawnPosition) ||
                !std::isfinite(player.health) || !std::isfinite(player.hunger) ||
                !std::isfinite(player.stamina) || player.health < 0.0f ||
                player.health > player.stats.maxHealth || player.hunger < 0.0f ||
                player.hunger > player.stats.maxHunger || player.stamina < 0.0f ||
                player.stamina > player.stats.maxStamina || (player.alive != (player.health > 0.0f)) ||
                (player.mode != GameMode::Creative && player.mode != GameMode::Survival))
                throw std::invalid_argument("Invalid player save state");
        }
    }


    void PlayerSave::write( const std::filesystem::path& path, const PlayerState& player ) {
        validatePlayer(player);
        Bytes data{ 'V', 'E', 'P', '1' };
        put32(data, 1);
        put32(data, player.mode == GameMode::Survival ? 1u : 0u);
        putVec3(data, player.position);
        putVec3(data, player.spawnPosition);
        putFloat(data, player.health);
        putFloat(data, player.hunger);
        putFloat(data, player.stamina);
        putFloat(data, player.stats.maxHealth);
        putFloat(data, player.stats.maxHunger);
        putFloat(data, player.stats.maxStamina);
        putFloat(data, player.stats.walkSpeed);
        putFloat(data, player.stats.sprintMultiplier);
        putFloat(data, player.stats.jumpSpeed);
        put32(data, player.alive ? 1u : 0u);
        put32(data, checksum(data, data.size()));

        if (!path.parent_path().empty())
            std::filesystem::create_directories(path.parent_path());
        const std::filesystem::path temporary = path.string() + ".tmp";
        {
            std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
            if (!file)
                throw std::runtime_error("Cannot open temporary player save");
            file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
            file.flush();
            if (!file)
                throw std::runtime_error("Failed writing temporary player save");
        }
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot replace player save atomically");
#else
        std::filesystem::rename(temporary, path);
#endif
    }

    bool PlayerSave::read( const std::filesystem::path& path, PlayerState& destination ) {
        if (!std::filesystem::exists(path))
            return false;
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Cannot open player save");
        constexpr std::size_t expectedSize = 80;
        Bytes data(expectedSize + 1);
        file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (file.bad())
            throw std::runtime_error("Cannot read player save");
        if (file.gcount() != static_cast<std::streamsize>(expectedSize))
            throw std::runtime_error("Invalid player save size");
        data.resize(expectedSize);
        if (data[0] != 'V' || data[1] != 'E' ||
            data[2] != 'P' || data[3] != '1')
            throw std::runtime_error("Invalid player save header or size");

        std::size_t offset = 4;
        if (get32(data, offset) != 1u)
            throw std::runtime_error("Unsupported player save version");
        const std::uint32_t mode = get32(data, offset);
        if (mode > 1)
            throw std::runtime_error("Invalid player game mode");

        PlayerState next = destination;
        next.position = getVec3(data, offset);
        next.previousPosition = next.position;
        next.spawnPosition = getVec3(data, offset);
        next.health = getFloat(data, offset);
        next.hunger = getFloat(data, offset);
        next.stamina = getFloat(data, offset);
        next.stats.maxHealth = getFloat(data, offset);
        next.stats.maxHunger = getFloat(data, offset);
        next.stats.maxStamina = getFloat(data, offset);
        next.stats.walkSpeed = getFloat(data, offset);
        next.stats.sprintMultiplier = getFloat(data, offset);
        next.stats.jumpSpeed = getFloat(data, offset);
        const std::uint32_t alive = get32(data, offset);
        if (alive > 1)
            throw std::runtime_error("Invalid player alive flag");
        next.alive = alive == 1;
        next.mode = mode == 1 ? GameMode::Survival : GameMode::Creative;
        next.velocity = Vec3{};
        next.grounded = false;
        next.spawned = true;
        const std::uint32_t storedChecksum = get32(data, offset);
        if (storedChecksum != checksum(data, data.size() - 4) || offset != data.size())
            throw std::runtime_error("Player save checksum mismatch");
        validatePlayer(next);
        destination = next;
        return true;
    }
}
