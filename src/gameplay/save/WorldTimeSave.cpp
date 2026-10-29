#include "gameplay/save/WorldTimeSave.h"

#include <bit>
#include <cstdint>
#include <fstream>
#include <iterator>
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
                data.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
        }

        void put64( Bytes& data, std::uint64_t value ) {
            for (int i = 0; i < 8; ++i)
                data.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
        }

        std::uint32_t get32( const Bytes& data, std::size_t& offset ) {
            if (offset > data.size() || data.size() - offset < 4)
                throw std::runtime_error("World time save truncated");
            std::uint32_t result = 0;
            for (int i = 0; i < 4; ++i)
                result |= std::uint32_t(data[offset++]) << (8 * i);
            return result;
        }

        std::uint64_t get64( const Bytes& data, std::size_t& offset ) {
            if (offset > data.size() || data.size() - offset < 8)
                throw std::runtime_error("World time save truncated");
            std::uint64_t result = 0;
            for (int i = 0; i < 8; ++i)
                result |= std::uint64_t(data[offset++]) << (8 * i);
            return result;
        }

        std::uint32_t checksum( const Bytes& bytes, std::size_t size ) {
            std::uint32_t result = 2166136261u;
            for (std::size_t i = 0; i < size; ++i) {
                result ^= bytes[i];
                result *= 16777619u;
            }
            return result;
        }
    }


    void WorldTimeSave::write( const std::filesystem::path& path, const WorldClock& clock ) {
        // Validate before touching the existing save file.
        WorldClock validated;
        validated.restore(clock.day(), clock.secondsOfDay(), clock.dayLengthSeconds());

        Bytes bytes{ 'V', 'E', 'T', '1' };
        put32(bytes, 1);
        put64(bytes, clock.day());
        put64(bytes, std::bit_cast<std::uint64_t>(clock.secondsOfDay()));
        put64(bytes, std::bit_cast<std::uint64_t>(clock.dayLengthSeconds()));
        put32(bytes, checksum(bytes, bytes.size()));

        if (!path.parent_path().empty())
            std::filesystem::create_directories(path.parent_path());
        std::filesystem::path temporary = path;
        temporary += ".tmp";
        {
            std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
            if (!stream)
                throw std::runtime_error("Cannot open temporary world time save");
            stream.write(reinterpret_cast<const char*>(bytes.data()),
                         static_cast<std::streamsize>(bytes.size()));
            stream.flush();
            if (!stream)
                throw std::runtime_error("Cannot write temporary world time save");
        }
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot replace world time save");
#else
        std::filesystem::rename(temporary, path);
#endif
    }

    bool WorldTimeSave::read( const std::filesystem::path& path, WorldClock& destination ) {
        if (!std::filesystem::exists(path))
            return false;

        // Reject malformed/oversized saves before allocating memory for their contents.
        if (std::filesystem::file_size(path) != 36)
            throw std::runtime_error("Invalid world time save size");

        std::ifstream stream(path, std::ios::binary);
        if (!stream)
            throw std::runtime_error("Cannot open world time save");
        Bytes bytes(36);
        stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!stream || stream.peek() != std::char_traits<char>::eof())
            throw std::runtime_error("Cannot read world time save");
        if (bytes[0] != 'V' || bytes[1] != 'E' || bytes[2] != 'T' || bytes[3] != '1')
            throw std::runtime_error("Invalid world time save header");

        std::size_t offset = 4;
        if (get32(bytes, offset) != 1)
            throw std::runtime_error("Unsupported world time save version");
        const std::uint64_t day = get64(bytes, offset);
        const double seconds = std::bit_cast<double>(get64(bytes, offset));
        const double dayLength = std::bit_cast<double>(get64(bytes, offset));
        const std::uint32_t stored = get32(bytes, offset);
        if (offset != bytes.size() || stored != checksum(bytes, bytes.size() - 4))
            throw std::runtime_error("World time save checksum mismatch");

        WorldClock candidate = destination;
        candidate.restore(day, seconds, dayLength);
        destination = candidate;
        return true;
    }
}
