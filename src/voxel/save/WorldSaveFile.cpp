#include "voxel/save/WorldSaveFile.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif


namespace ve
{
    namespace
    {
        constexpr std::array<std::uint8_t, 8> Magic{ 'V', 'E', 'W', 'O', 'R', 'L', 'D', '1' };
        constexpr std::uint32_t FormatVersion = 1;
        constexpr std::uint32_t GeneratorVersion = 1;
        constexpr std::size_t MaxFileBytes = 64u * 1024u * 1024u;
        constexpr std::uint32_t MaxSavedChunks = 100000;
        constexpr std::uint64_t FnvOffset = 14695981039346656037ULL;
        constexpr std::uint64_t FnvPrime = 1099511628211ULL;

        static_assert(sizeof(int) == 4);
        static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559);
        static_assert(Chunk::BlockCount <= std::numeric_limits<std::uint16_t>::max());


        void appendUnsigned( std::vector<std::uint8_t>& bytes, std::uint64_t value, int count ) {
            for (int i = 0; i < count; ++i) {
                bytes.push_back(static_cast<std::uint8_t>(value & 0xFFu));
                value >>= 8;
            }
        }

        std::uint64_t readUnsigned( const std::vector<std::uint8_t>& bytes, std::size_t& cursor,
                                    std::size_t end, int count ) {
            if (cursor > end || static_cast<std::size_t>(count) > end - cursor)
                throw std::runtime_error("Truncated World Save file");

            std::uint64_t value = 0;

            for (int i = 0; i < count; ++i)
                value |= static_cast<std::uint64_t>(bytes[cursor++]) << (8 * i);

            return value;
        }

        void appendSigned( std::vector<std::uint8_t>& bytes, int value ) {
            appendUnsigned(bytes, std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(value)), 4);
        }

        int readSigned( const std::vector<std::uint8_t>& bytes, std::size_t& cursor, std::size_t end ) {
            const auto bits = static_cast<std::uint32_t>(readUnsigned(bytes, cursor, end, 4));
            return static_cast<int>(std::bit_cast<std::int32_t>(bits));
        }

        void appendDouble( std::vector<std::uint8_t>& bytes, double value ) {
            appendUnsigned(bytes, std::bit_cast<std::uint64_t>(value), 8);
        }

        double readDouble( const std::vector<std::uint8_t>& bytes, std::size_t& cursor, std::size_t end ) {
            return std::bit_cast<double>(readUnsigned(bytes, cursor, end, 8));
        }

        std::uint64_t checksum( const std::vector<std::uint8_t>& bytes, std::size_t end ) {
            std::uint64_t result = FnvOffset;

            for (std::size_t i = 0; i < end; ++i) {
                result ^= bytes[i];
                result *= FnvPrime;
            }

            return result;
        }

        void validateTerrain( const TerrainGenerationConfig& config ) {
            if (config.dirtDepth < 0 || !std::isfinite(config.heightAmplitude) || config.heightAmplitude < 0.0)
                throw std::runtime_error("Invalid World Save terrain settings");

            if (config.noise.octaves < 1 || config.noise.octaves > 12 ||
                !std::isfinite(config.noise.frequency) || config.noise.frequency <= 0.0 ||
                !std::isfinite(config.noise.lacunarity) || config.noise.lacunarity < 1.0 ||
                !std::isfinite(config.noise.persistence) || config.noise.persistence <= 0.0 ||
                config.noise.persistence > 1.0)
                throw std::runtime_error("Invalid World Save noise settings");
        }

        bool validBlockId( std::uint8_t value ) {
            return value <= static_cast<std::uint8_t>(BlockId::Stone);
        }

        void validatePosition( const ChunkPosition& position ) {
            const std::int64_t low = std::numeric_limits<int>::min();
            const std::int64_t high = std::numeric_limits<int>::max();

            for (int coordinate : { position.x, position.y, position.z }) {
                const std::int64_t first = static_cast<std::int64_t>(coordinate) * Chunk::Size;

                if (first < low || first + Chunk::Size - 1 > high)
                    throw std::runtime_error("World Save contains out-of-range Chunk coordinates");
            }
        }

        void replaceFile( const std::filesystem::path& temporary, const std::filesystem::path& destination ) {
#if defined(_WIN32)
            if (!MoveFileExW(temporary.c_str(), destination.c_str(),
                             MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                throw std::runtime_error("Failed to replace World Save file");
#else
            std::filesystem::rename(temporary, destination);
#endif
        }
    }


    void writeWorldSaveFile( const std::filesystem::path& path, const WorldSaveData& data ) {
        validateTerrain(data.terrain);

        if (data.deltas.size() > MaxSavedChunks)
            throw std::runtime_error("Too many saved Chunks");

        std::vector<std::uint8_t> bytes{};
        bytes.insert(bytes.end(), Magic.begin(), Magic.end());
        appendUnsigned(bytes, FormatVersion, 4);
        appendUnsigned(bytes, GeneratorVersion, 4);

        const TerrainGenerationConfig& config = data.terrain;
        appendUnsigned(bytes, config.seed, 8);
        appendSigned(bytes, config.surfaceY);
        appendSigned(bytes, config.dirtDepth);
        appendDouble(bytes, config.heightAmplitude);
        appendSigned(bytes, config.noise.octaves);
        appendDouble(bytes, config.noise.frequency);
        appendDouble(bytes, config.noise.lacunarity);
        appendDouble(bytes, config.noise.persistence);
        appendUnsigned(bytes, data.deltas.size(), 4);

        for (const auto& entry : data.deltas) {
            const ChunkPosition& position = entry.first;
            const ChunkDelta& delta = entry.second;
            validatePosition(position);

            if (delta.blocks.empty() || delta.blocks.size() > Chunk::BlockCount)
                throw std::runtime_error("Invalid Delta count for a saved Chunk");

            appendSigned(bytes, position.x);
            appendSigned(bytes, position.y);
            appendSigned(bytes, position.z);
            appendUnsigned(bytes, delta.blocks.size(), 4);

            std::array<bool, Chunk::BlockCount> seen{};

            for (const BlockDelta& block : delta.blocks) {
                const auto type = static_cast<std::uint8_t>(block.type);

                if (block.index >= Chunk::BlockCount || seen[block.index] || !validBlockId(type))
                    throw std::runtime_error("Invalid or duplicate Block Delta");

                seen[block.index] = true;
                appendUnsigned(bytes, block.index, 2);
                appendUnsigned(bytes, type, 1);
            }

            if (bytes.size() + 8 > MaxFileBytes)
                throw std::runtime_error("World Save exceeds maximum file size");
        }

        appendUnsigned(bytes, checksum(bytes, bytes.size()), 8);

        const std::filesystem::path parent = path.parent_path();

        if (!parent.empty())
            std::filesystem::create_directories(parent);

        std::filesystem::path temporary = path;
        temporary += ".tmp";

        try {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);

            if (!output)
                throw std::runtime_error("Cannot open temporary World Save for writing");

            output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            output.flush();

            if (!output)
                throw std::runtime_error("Failed to write World Save");

            output.close();

            if (!output)
                throw std::runtime_error("Failed to close World Save");

            replaceFile(temporary, path);
        } catch (...) {
            std::error_code ignored{};
            std::filesystem::remove(temporary, ignored);
            throw;
        }
    }


    WorldSaveData readWorldSaveFile( const std::filesystem::path& path ) {
        const std::uintmax_t size = std::filesystem::file_size(path);

        if (size < Magic.size() + 4 + 4 + 8 || size > MaxFileBytes)
            throw std::runtime_error("Invalid World Save file size");

        std::ifstream input(path, std::ios::binary);

        if (!input)
            throw std::runtime_error("Cannot open World Save for reading");

        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

        if (!input)
            throw std::runtime_error("Failed to read complete World Save");

        const std::size_t contentEnd = bytes.size() - 8;
        std::size_t checksumCursor = contentEnd;
        const std::uint64_t storedChecksum = readUnsigned(bytes, checksumCursor, bytes.size(), 8);

        if (storedChecksum != checksum(bytes, contentEnd))
            throw std::runtime_error("World Save checksum mismatch");

        std::size_t cursor = 0;

        for (std::uint8_t expected : Magic) {
            if (readUnsigned(bytes, cursor, contentEnd, 1) != expected)
                throw std::runtime_error("Invalid World Save magic");
        }

        if (readUnsigned(bytes, cursor, contentEnd, 4) != FormatVersion)
            throw std::runtime_error("Unsupported World Save format version");

        if (readUnsigned(bytes, cursor, contentEnd, 4) != GeneratorVersion)
            throw std::runtime_error("Unsupported terrain generator version");

        WorldSaveData result{};
        TerrainGenerationConfig& config = result.terrain;
        config.seed = readUnsigned(bytes, cursor, contentEnd, 8);
        config.surfaceY = readSigned(bytes, cursor, contentEnd);
        config.dirtDepth = readSigned(bytes, cursor, contentEnd);
        config.heightAmplitude = readDouble(bytes, cursor, contentEnd);
        config.noise.octaves = readSigned(bytes, cursor, contentEnd);
        config.noise.frequency = readDouble(bytes, cursor, contentEnd);
        config.noise.lacunarity = readDouble(bytes, cursor, contentEnd);
        config.noise.persistence = readDouble(bytes, cursor, contentEnd);
        validateTerrain(config);

        const auto chunkCount = static_cast<std::uint32_t>(readUnsigned(bytes, cursor, contentEnd, 4));

        if (chunkCount > MaxSavedChunks)
            throw std::runtime_error("World Save has too many Chunks");

        for (std::uint32_t i = 0; i < chunkCount; ++i) {
            const ChunkPosition position{
                readSigned(bytes, cursor, contentEnd), readSigned(bytes, cursor, contentEnd),
                readSigned(bytes, cursor, contentEnd)
            };
            validatePosition(position);

            const auto blockCount = static_cast<std::uint32_t>(readUnsigned(bytes, cursor, contentEnd, 4));

            if (blockCount == 0 || blockCount > Chunk::BlockCount)
                throw std::runtime_error("World Save contains invalid Delta count");

            ChunkDelta delta{};
            delta.blocks.reserve(blockCount);
            std::array<bool, Chunk::BlockCount> seen{};

            for (std::uint32_t j = 0; j < blockCount; ++j) {
                const auto index = static_cast<std::uint16_t>(readUnsigned(bytes, cursor, contentEnd, 2));
                const auto type = static_cast<std::uint8_t>(readUnsigned(bytes, cursor, contentEnd, 1));

                if (index >= Chunk::BlockCount || seen[index] || !validBlockId(type))
                    throw std::runtime_error("World Save contains invalid Block Delta");

                seen[index] = true;
                delta.blocks.push_back(BlockDelta{ index, static_cast<BlockId>(type) });
            }

            if (!result.deltas.emplace(position, std::move(delta)).second)
                throw std::runtime_error("World Save contains duplicate Chunk coordinates");
        }

        if (cursor != contentEnd)
            throw std::runtime_error("World Save contains unexpected extra data");

        return result;
    }
}