#pragma once

#include "math/Color.h"
#include "voxel/block/BlockId.h"


namespace ve
{
    struct BlockDefinition
    {
        BlockId id = BlockId::Air;
        bool solid = false;
        Color32 tint = makeColor(255, 255, 255);
    };
}
