#include "world/VoxelRaycaster.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>

#include "world/World.h"



namespace ve
{
    namespace
    {
        struct AxisTraversal
        {
            std::int64_t step = 0;

            double tMax = std::numeric_limits<double>::infinity();
            double tDelta = std::numeric_limits<double>::infinity();
        };



        AxisTraversal createAxisTraversal( double origin, double direction, std::int64_t voxel ) {
            if (direction > 0.0) {
                const double nextBoundary = static_cast<double>(voxel + 1);

                return AxisTraversal{
                    1,
                    (nextBoundary - origin) / direction,
                    1.0 / direction
                };
            }

            if (direction < 0.0) {
                const double nextBoundary = static_cast<double>(voxel);

                return AxisTraversal{
                    -1,
                    (origin - nextBoundary) / -direction,
                    1.0 / -direction
                };
            }


            return AxisTraversal{};
        }



        double initialVoxelCoordinate( double origin, double direction ) {
            double voxel = std::floor(origin);



            // At an Exact Boundary, the Negative Ray Starts in the Negative Cell

            if (direction < 0.0 && origin == voxel) {
                voxel -= 1.0;
            }


            return voxel;
        }



        bool isValidBlockCoordinate( std::int64_t value ) {
            return value >= std::numeric_limits<int>::min() &&
                   value <= std::numeric_limits<int>::max();
        }
    }



    std::optional<VoxelRaycastHit> VoxelRaycaster::cast(
        const World& world,
        const Vec3& origin,
        const Vec3& direction,
        float maxDistance
    ) {
        // Validate Ray Origin

        if (!std::isfinite(origin.x) ||
            !std::isfinite(origin.y) ||
            !std::isfinite(origin.z)) {
            throw std::invalid_argument("Voxel Ray origin must be finite");
        }



        // Validate Ray Direction

        if (!std::isfinite(direction.x) ||
            !std::isfinite(direction.y) ||
            !std::isfinite(direction.z)) {
            throw std::invalid_argument("Voxel Ray direction must be finite");
        }



        // Validate Ray Reach

        if (!std::isfinite(maxDistance) || maxDistance < 0.0f) {
            throw std::invalid_argument("Voxel Ray distance must be nonnegative and finite");
        }



        // Normalize the Direction Using Double Precision

        double dx = static_cast<double>(direction.x);
        double dy = static_cast<double>(direction.y);
        double dz = static_cast<double>(direction.z);

        const double lengthSquared = dx * dx + dy * dy + dz * dz;

        if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0) {
            throw std::invalid_argument("Voxel Ray direction must be nonzero");
        }


        const double inverseLength = 1.0 / std::sqrt(lengthSquared);

        dx *= inverseLength;
        dy *= inverseLength;
        dz *= inverseLength;



        const double ox = static_cast<double>(origin.x);
        const double oy = static_cast<double>(origin.y);
        const double oz = static_cast<double>(origin.z);



        // Find the First Voxel Containing the Ray Origin

        const double firstX = initialVoxelCoordinate(ox, dx);
        const double firstY = initialVoxelCoordinate(oy, dy);
        const double firstZ = initialVoxelCoordinate(oz, dz);



        // WorldBlockPosition Uses Signed 32-bit-style Integer Coordinates

        const double minimum = static_cast<double>(std::numeric_limits<int>::min());
        const double maximum = static_cast<double>(std::numeric_limits<int>::max());

        if (firstX < minimum || firstX > maximum ||
            firstY < minimum || firstY > maximum ||
            firstZ < minimum || firstZ > maximum) {
            return std::nullopt;
        }



        std::int64_t voxelX = static_cast<std::int64_t>(firstX);
        std::int64_t voxelY = static_cast<std::int64_t>(firstY);
        std::int64_t voxelZ = static_cast<std::int64_t>(firstZ);



        // Prepare DDA Traversal for Each Axis

        AxisTraversal axisX = createAxisTraversal(ox, dx, voxelX);
        AxisTraversal axisY = createAxisTraversal(oy, dy, voxelY);
        AxisTraversal axisZ = createAxisTraversal(oz, dz, voxelZ);



        double traveled = 0.0;

        WorldBlockPosition faceNormal{ 0, 0, 0 };

        const double maximumDistance = static_cast<double>(maxDistance);



        // Visit Voxels in Ray Order

        while (traveled <= maximumDistance) {
            if (!isValidBlockCoordinate(voxelX) ||
                !isValidBlockCoordinate(voxelY) ||
                !isValidBlockCoordinate(voxelZ)) {
                return std::nullopt;
            }



            const WorldBlockPosition position{
                static_cast<int>(voxelX),
                static_cast<int>(voxelY),
                static_cast<int>(voxelZ)
            };



            // Check the Current Voxel

            const BlockType type = world.blockAt(position);

            if (type != BlockType::Air) {
                return VoxelRaycastHit{
                    position,
                    faceNormal,
                    type,
                    static_cast<float>(traveled)
                };
            }



            // Find the Next Grid Boundary Along the Ray

            const double nextDistance = std::min({
                axisX.tMax,
                axisY.tMax,
                axisZ.tMax
            });



            // No More Voxels Are Reachable Within the Maximum Distance

            if (!std::isfinite(nextDistance) || nextDistance > maximumDistance) {
                break;
            }



            // Advance Along the Axis with the Nearest Boundary

            if (axisX.tMax <= axisY.tMax && axisX.tMax <= axisZ.tMax) {
                traveled = axisX.tMax;

                voxelX += axisX.step;

                faceNormal = WorldBlockPosition{
                    static_cast<int>(-axisX.step),
                    0,
                    0
                };

                axisX.tMax += axisX.tDelta;
            }
            else if (axisY.tMax <= axisZ.tMax) {
                traveled = axisY.tMax;

                voxelY += axisY.step;

                faceNormal = WorldBlockPosition{
                    0,
                    static_cast<int>(-axisY.step),
                    0
                };

                axisY.tMax += axisY.tDelta;
            }
            else {
                traveled = axisZ.tMax;

                voxelZ += axisZ.step;

                faceNormal = WorldBlockPosition{
                    0,
                    0,
                    static_cast<int>(-axisZ.step)
                };

                axisZ.tMax += axisZ.tDelta;
            }
        }



        return std::nullopt;
    }
}