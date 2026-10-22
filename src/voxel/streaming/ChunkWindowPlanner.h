#pragma once

#include <vector>

#include "math/Vec3.h"
#include "voxel/chunk/VoxelStorage.h"


namespace ve
{
    struct ChunkWindowPlan
    {
        std::vector<ChunkPosition> toLoad{};
        std::vector<ChunkPosition> toUnload{};
    };

    ChunkPosition horizontalChunkAt( const Vec3& cameraPosition );

    ChunkWindowPlan planHorizontalChunkWindow( const Vec3& cameraPosition, int radius,
                                               const VoxelStorage& storage );
}