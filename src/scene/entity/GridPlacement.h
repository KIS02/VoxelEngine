#pragma once

#include "voxel/chunk/ChunkCoordinates.h"


namespace ve
{
    // Optional grid attachment for an ordinary Entity (not a separate BlockEntity type).
    struct GridPlacement
    {
        WorldBlockPosition position{};
    };
}
