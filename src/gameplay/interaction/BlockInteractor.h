#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>

#include "math/Vec3.h"
#include "voxel/block/BlockId.h"
#include "voxel/ray/VoxelRaycaster.h"
#include "voxel/chunk/VoxelStorage.h"



namespace ve
{
    struct BlockPlacementResult
    {
        WorldBlockPosition placedBlock{};
        WorldBlockPosition attachedTo{};
        WorldBlockPosition faceNormal{};

        BlockId type = BlockId::Air;
    };



    class BlockInteractor {
        private:
            static bool isValidCoordinate( std::int64_t coordinate ) {
                return coordinate >= std::numeric_limits<int>::min() &&
                       coordinate <= std::numeric_limits<int>::max();
            }



            static bool isValidFaceNormal( const WorldBlockPosition& normal ) {
                if (normal.x < -1 || normal.x > 1 ||
                    normal.y < -1 || normal.y > 1 ||
                    normal.z < -1 || normal.z > 1) {
                    return false;
                }

                return std::abs(normal.x) + std::abs(normal.y) + std::abs(normal.z) == 1;
            }



            static bool isCameraInsideBlock( const Vec3& origin, const WorldBlockPosition& block ) {
                return std::floor(static_cast<double>(origin.x)) == static_cast<double>(block.x) &&
                       std::floor(static_cast<double>(origin.y)) == static_cast<double>(block.y) &&
                       std::floor(static_cast<double>(origin.z)) == static_cast<double>(block.z);
            }


        public:
            static std::optional<VoxelRaycastHit> breakBlock(
                VoxelStorage& world, const Vec3& origin,
                const Vec3& direction, float maxDistance
            ) {
                const std::optional<VoxelRaycastHit> hit = VoxelRaycaster::cast(
                    world, origin, direction, maxDistance
                );

                if (!hit.has_value()) {
                    return std::nullopt;
                }



                world.setBlock(hit->block, BlockId::Air);

                return hit;
            }



            static std::optional<BlockPlacementResult> placeBlock(
                VoxelStorage& world, const Vec3& origin, const Vec3& direction,
                float maxDistance, BlockId type
            ) {
                if (type == BlockId::Air) {
                    throw std::invalid_argument("Cannot place an Air block");
                }



                // Find the Nearest Solid Block

                const std::optional<VoxelRaycastHit> hit = VoxelRaycaster::cast(
                    world, origin, direction, maxDistance
                );

                if (!hit.has_value()) {
                    return std::nullopt;
                }



                // A Ray Starting Inside a Block Has No Entry Face

                const WorldBlockPosition& normal = hit->faceNormal;

                if (!isValidFaceNormal(normal)) {
                    return std::nullopt;
                }



                // Calculate the Neighboring Block Position Without Integer Overflow

                const std::int64_t targetX =
                    static_cast<std::int64_t>(hit->block.x) + normal.x;

                const std::int64_t targetY =
                    static_cast<std::int64_t>(hit->block.y) + normal.y;

                const std::int64_t targetZ =
                    static_cast<std::int64_t>(hit->block.z) + normal.z;



                if (!isValidCoordinate(targetX) ||
                    !isValidCoordinate(targetY) ||
                    !isValidCoordinate(targetZ)) {
                    return std::nullopt;
                }



                const WorldBlockPosition target{
                    static_cast<int>(targetX),
                    static_cast<int>(targetY),
                    static_cast<int>(targetZ)
                };



                // Never Overwrite an Existing Solid Block

                if (world.blockAt(target) != BlockId::Air) {
                    return std::nullopt;
                }



                // Prevent Placing a Block Around the Camera's Current Position

                if (isCameraInsideBlock(origin, target)) {
                    return std::nullopt;
                }



                // Update the VoxelStorage and Schedule Dirty Chunk Rebuilding

                world.setBlock(target, type);



                return BlockPlacementResult{
                    target,
                    hit->block,
                    normal,
                    type
                };
            }
    };
}