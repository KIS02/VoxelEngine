#include "voxel/generation/CoordinateHash.h"


namespace ve
{
    std::uint32_t coordinateHash( WorldSeed seed, int worldX, int worldZ ) {
        const std::uint64_t x = static_cast<std::uint32_t>(worldX);
        const std::uint64_t z = static_cast<std::uint32_t>(worldZ);

        std::uint64_t value = seed + 0x9E3779B97F4A7C15ULL;
        value ^= x * 0x9E3779B185EBCA87ULL;
        value ^= z * 0xC2B2AE3D27D4EB4FULL;

        value ^= value >> 30;
        value *= 0xBF58476D1CE4E5B9ULL;
        value ^= value >> 27;
        value *= 0x94D049BB133111EBULL;
        value ^= value >> 31;

        return static_cast<std::uint32_t>(value >> 32);
    }


    float latticeValue( WorldSeed seed, int worldX, int worldZ ) {
        const std::uint32_t hash = coordinateHash(seed, worldX, worldZ);

        return static_cast<float>(hash >> 8) * (1.0f / 16777216.0f);
    }
}