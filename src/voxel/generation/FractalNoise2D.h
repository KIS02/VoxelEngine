#pragma once

#include "voxel/generation/TerrainGenerationConfig.h"


namespace ve
{
    double fractalNoise2D( WorldSeed seed, double worldX, double worldZ,
                           const FractalNoiseConfig& config );
}