#include "voxel/streaming/ChunkWindowPlanner.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>


namespace ve
{
    namespace
    {
        int checkedBlockCoordinate( float coordinate ) {
            if (!std::isfinite(coordinate))
                throw std::invalid_argument("Camera coordinates must be finite");

            const double floored = std::floor(static_cast<double>(coordinate));
            const double minimum = static_cast<double>(std::numeric_limits<int>::min());
            const double maximumExclusive = static_cast<double>(std::numeric_limits<int>::max()) + 1.0;

            if (floored < minimum || floored >= maximumExclusive)
                throw std::out_of_range("Camera block coordinate is outside integer range");

            return static_cast<int>(floored);
        }
    }


    ChunkPosition horizontalChunkAt( const Vec3& cameraPosition ) {
        const WorldBlockPosition cameraBlock{
            checkedBlockCoordinate(cameraPosition.x), 0, checkedBlockCoordinate(cameraPosition.z)
        };

        return worldToChunkLocation(cameraBlock).chunk;
    }


    ChunkWindowPlan planHorizontalChunkWindow( const Vec3& cameraPosition, int radius,
                                               const VoxelStorage& storage ) {
        if (radius < 0 || radius > 8)
            throw std::invalid_argument("Chunk radius must be between 0 and 8");

        const ChunkPosition center = horizontalChunkAt(cameraPosition);

        std::set<ChunkPosition, ChunkPositionLess> desired{};
        const std::int64_t minimum = std::numeric_limits<int>::min();
        const std::int64_t maximum = std::numeric_limits<int>::max();

        for (int dz = -radius; dz <= radius; ++dz) {
            for (int dx = -radius; dx <= radius; ++dx) {
                const std::int64_t chunkX = static_cast<std::int64_t>(center.x) + dx;
                const std::int64_t chunkZ = static_cast<std::int64_t>(center.z) + dz;
                const std::int64_t firstX = chunkX * Chunk::Size;
                const std::int64_t lastX = firstX + Chunk::Size - 1;
                const std::int64_t firstZ = chunkZ * Chunk::Size;
                const std::int64_t lastZ = firstZ + Chunk::Size - 1;

                if (firstX < minimum || lastX > maximum || firstZ < minimum || lastZ > maximum)
                    throw std::out_of_range("Desired Chunk exceeds supported world coordinates");

                desired.insert(ChunkPosition{ static_cast<int>(chunkX), 0, static_cast<int>(chunkZ) });
            }
        }

        ChunkWindowPlan plan{};
        std::set<ChunkPosition, ChunkPositionLess> missing = desired;

        for (const auto& entry : storage.chunks()) {
            const ChunkPosition& position = entry.first;
            const ChunkPosition column{ position.x, 0, position.z };

            if (desired.find(column) == desired.end())
                plan.toUnload.push_back(position);
            else if (position.y == 0)
                missing.erase(column);
        }

        plan.toLoad.assign(missing.begin(), missing.end());

        return plan;
    }
}