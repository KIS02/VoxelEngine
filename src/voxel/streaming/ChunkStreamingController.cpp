#include "voxel/streaming/ChunkStreamingController.h"

#include <bit>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <utility>

#include "voxel/save/WorldSaveFile.h"
#include "voxel/streaming/ChunkWindowPlanner.h"


namespace ve
{
    namespace
    {
        bool sameTerrainSettings( const TerrainGenerationConfig& a, const TerrainGenerationConfig& b ) {
            return a.seed == b.seed && a.surfaceY == b.surfaceY && a.dirtDepth == b.dirtDepth &&
                   std::bit_cast<std::uint64_t>(a.heightAmplitude) == std::bit_cast<std::uint64_t>(b.heightAmplitude) &&
                   a.noise.octaves == b.noise.octaves &&
                   std::bit_cast<std::uint64_t>(a.noise.frequency) == std::bit_cast<std::uint64_t>(b.noise.frequency) &&
                   std::bit_cast<std::uint64_t>(a.noise.lacunarity) == std::bit_cast<std::uint64_t>(b.noise.lacunarity) &&
                   std::bit_cast<std::uint64_t>(a.noise.persistence) == std::bit_cast<std::uint64_t>(b.noise.persistence);
        }
    }


    ChunkStreamingController::ChunkStreamingController( VoxelWorld& world, TerrainGenerationConfig config, int radius )
        : world_(world), generator_(config), radius_(radius) {
        if (radius_ < 0 || radius_ > 8)
            throw std::invalid_argument("Chunk streaming radius must be between 0 and 8");
    }


    ChunkStreamingResult ChunkStreamingController::update( const Vec3& cameraPosition ) {
        const ChunkPosition center = horizontalChunkAt(cameraPosition);

        if (lastCenter_.has_value() &&
            lastCenter_->x == center.x && lastCenter_->z == center.z)
            return ChunkStreamingResult{};

        const ChunkWindowPlan plan = planHorizontalChunkWindow(cameraPosition, radius_, world_.blocks());
        ChunkStreamingResult result{};
        result.changed = true;


        for (const ChunkPosition& position : plan.toUnload) {
            const Chunk* chunk = world_.blocks().findChunk(position);

            if (chunk == nullptr)
                continue;

            const Chunk baseline = position.y == 0 ? generator_.generate(position) : Chunk{};
            ChunkDelta delta = createChunkDelta(*chunk, baseline);

            if (!delta.blocks.empty())
                cachedDeltas_.insert_or_assign(position, std::move(delta));
            else
                cachedDeltas_.erase(position);

            if (world_.removeChunk(position))
                ++result.unloaded;
        }


        const std::int64_t minX = static_cast<std::int64_t>(center.x) - radius_;
        const std::int64_t maxX = static_cast<std::int64_t>(center.x) + radius_;
        const std::int64_t minZ = static_cast<std::int64_t>(center.z) - radius_;
        const std::int64_t maxZ = static_cast<std::int64_t>(center.z) + radius_;

        std::set<ChunkPosition, ChunkPositionLess> loadPositions(
            plan.toLoad.begin(), plan.toLoad.end()
        );

        for (const auto& entry : cachedDeltas_) {
            const ChunkPosition& position = entry.first;

            if (position.y == 0 || position.x < minX || position.x > maxX ||
                position.z < minZ || position.z > maxZ)
                continue;

            if (world_.blocks().findChunk(position) == nullptr)
                loadPositions.insert(position);
        }


        for (const ChunkPosition& position : loadPositions) {
            const auto cached = cachedDeltas_.find(position);

            if (cached != cachedDeltas_.end()) {
                Chunk restored = position.y == 0 ? generator_.generate(position) : Chunk{};

                applyChunkDelta(restored, cached->second);

                world_.blocks().setChunk(position, std::move(restored));
                cachedDeltas_.erase(cached);
                ++result.restored;
            } else {
                world_.blocks().setChunk(position, generator_.generate(position));
                ++result.generated;
            }
        }


        lastCenter_ = center;
        return result;
    }


    std::size_t ChunkStreamingController::cachedChunkCount() const {
        return cachedDeltas_.size();
    }


    ChunkCacheStatistics ChunkStreamingController::cacheStatistics() const {
        return calculateChunkCacheStatistics(cachedDeltas_);
    }


    bool ChunkStreamingController::loadWorld( const std::filesystem::path& path ) {
        if (lastCenter_.has_value() || world_.blocks().chunkCount() != 0 || !cachedDeltas_.empty())
            throw std::logic_error("World Save can only load before the first Streaming update");

        if (!std::filesystem::exists(path))
            return false;

        WorldSaveData saved = readWorldSaveFile(path);

        if (!sameTerrainSettings(saved.terrain, generator_.config()))
            throw std::runtime_error("World Save terrain settings do not match current generator");

        cachedDeltas_ = std::move(saved.deltas);
        return true;
    }


    void ChunkStreamingController::saveWorld( const std::filesystem::path& path ) const {
        WorldSaveData data{};
        data.terrain = generator_.config();
        data.deltas = cachedDeltas_;

        for (const auto& entry : world_.blocks().chunks()) {
            const ChunkPosition& position = entry.first;
            const Chunk& current = entry.second;
            const Chunk baseline = position.y == 0 ? generator_.generate(position) : Chunk{};
            ChunkDelta delta = createChunkDelta(current, baseline);

            if (delta.blocks.empty())
                data.deltas.erase(position);
            else
                data.deltas.insert_or_assign(position, std::move(delta));
        }

        writeWorldSaveFile(path, data);
    }
}