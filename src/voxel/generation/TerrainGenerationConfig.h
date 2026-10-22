#pragma once

#include <cstdint>


namespace ve
{
    using WorldSeed = std::uint64_t;


    struct FractalNoiseConfig
    {
        int octaves = 4;
        double frequency = 0.05;
        double lacunarity = 2.0;
        double persistence = 0.5;
    };


    struct TerrainGenerationConfig
    {
        WorldSeed seed = 1337;
        int surfaceY = 3;
        int dirtDepth = 2;
        double heightAmplitude = 12.0;
        FractalNoiseConfig noise{};
    };
}