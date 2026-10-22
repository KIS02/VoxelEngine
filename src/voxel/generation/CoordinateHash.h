#pragma once

#include <cstdint>

#include "voxel/generation/TerrainGenerationConfig.h"


namespace ve
{
    std::uint32_t coordinateHash( WorldSeed seed, int worldX, int worldZ );

    float latticeValue( WorldSeed seed, int worldX, int worldZ );
}