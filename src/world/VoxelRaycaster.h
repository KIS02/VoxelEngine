#pragma once

#include <optional>

#include "math/Vec3.h"
#include "world/BlockType.h"
#include "world/ChunkCoordinates.h"



namespace ve
{
    class World;



    struct VoxelRaycastHit
    {
        WorldBlockPosition block{};
        WorldBlockPosition faceNormal{};

        BlockType type = BlockType::Air;

        float distance = 0.0f;
    };



    class VoxelRaycaster {
        public:
            static std::optional<VoxelRaycastHit> cast(
                const World& world,
                const Vec3& origin,
                const Vec3& direction,
                float maxDistance
            );
    };
}