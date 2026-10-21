#pragma once

#include <optional>

#include "math/Vec3.h"
#include "voxel/block/BlockId.h"
#include "voxel/chunk/ChunkCoordinates.h"



namespace ve
{
    class VoxelStorage;



    struct VoxelRaycastHit
    {
        WorldBlockPosition block{};
        WorldBlockPosition faceNormal{};

        BlockId type = BlockId::Air;

        float distance = 0.0f;
    };



    class VoxelRaycaster {
        public:
            static std::optional<VoxelRaycastHit> cast(
                const VoxelStorage& world,
                const Vec3& origin,
                const Vec3& direction,
                float maxDistance
            );
    };
}