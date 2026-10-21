#include "world/World.h"

#include <cstdint>
#include <limits>



namespace ve
{
    Chunk& World::getOrCreateChunk( const ChunkPosition& position ) {
        auto [iterator, inserted] = chunks_.try_emplace(position);


        if (inserted) {
            markChunkDirty(position);
        }


        return iterator->second;
    }



    Chunk* World::findChunk( const ChunkPosition& position ) {
        const auto found = chunks_.find(position);

        if (found == chunks_.end()) {
            return nullptr;
        }


        return &found->second;
    }



    const Chunk* World::findChunk( const ChunkPosition& position ) const {
        const auto found = chunks_.find(position);

        if (found == chunks_.end()) {
            return nullptr;
        }


        return &found->second;
    }



    bool World::removeChunk( const ChunkPosition& position ) {
        const auto found = chunks_.find(position);


        // Removing a Missing Chunk Changes Nothing

        if (found == chunks_.end()) {
            return false;
        }



        // Remove the Actual Chunk Data

        chunks_.erase(found);



        // A Removed Chunk No Longer Needs Rebuilding

        dirtyChunks_.erase(position);



        // Existing Neighbors May Have Newly Exposed Faces

        markExistingNeighborDirty(position, -1, 0, 0);
        markExistingNeighborDirty(position,  1, 0, 0);

        markExistingNeighborDirty(position, 0, -1, 0);
        markExistingNeighborDirty(position, 0,  1, 0);

        markExistingNeighborDirty(position, 0, 0, -1);
        markExistingNeighborDirty(position, 0, 0,  1);


        return true;
    }



    BlockType World::blockAt( const WorldBlockPosition& position ) const {
        const ChunkBlockLocation location = worldToChunkLocation(position);

        const Chunk* chunk = findChunk(location.chunk);


        if (chunk == nullptr) {
            return BlockType::Air;
        }


        return chunk->blockAt(
            location.local.x,
            location.local.y,
            location.local.z
        );
    }



    void World::markExistingNeighborDirty( const ChunkPosition& position, int dx, int dy, int dz ) {
        const std::int64_t x = static_cast<std::int64_t>(position.x) + dx;
        const std::int64_t y = static_cast<std::int64_t>(position.y) + dy;
        const std::int64_t z = static_cast<std::int64_t>(position.z) + dz;


        constexpr std::int64_t minimum = std::numeric_limits<int>::min();
        constexpr std::int64_t maximum = std::numeric_limits<int>::max();


        if (x < minimum || x > maximum ||
            y < minimum || y > maximum ||
            z < minimum || z > maximum) {
            return;
        }


        const ChunkPosition neighbor{
            static_cast<int>(x),
            static_cast<int>(y),
            static_cast<int>(z)
        };


        if (findChunk(neighbor) != nullptr) {
            dirtyChunks_.insert(neighbor);
        }
    }



    void World::markChunkDirty( const ChunkPosition& position ) {
        if (findChunk(position) == nullptr) {
            return;
        }



        dirtyChunks_.insert(position);



        // Explicit Chunk Invalidation Also Affects Direct Neighbors

        markExistingNeighborDirty(position, -1, 0, 0);
        markExistingNeighborDirty(position,  1, 0, 0);

        markExistingNeighborDirty(position, 0, -1, 0);
        markExistingNeighborDirty(position, 0,  1, 0);

        markExistingNeighborDirty(position, 0, 0, -1);
        markExistingNeighborDirty(position, 0, 0,  1);
    }



    void World::setBlock( const WorldBlockPosition& position, BlockType type ) {
        const ChunkBlockLocation location = worldToChunkLocation(position);

        Chunk& chunk = getOrCreateChunk(location.chunk);



        // Ignore Unchanged Block Values

        if (chunk.blockAt(location.local.x, location.local.y, location.local.z) == type) {
            return;
        }



        chunk.setBlock(
            location.local.x,
            location.local.y,
            location.local.z,
            type
        );


        dirtyChunks_.insert(location.chunk);



        // Only Boundary Edits Affect Neighbor Chunk Meshes

        if (location.local.x == 0) {
            markExistingNeighborDirty(location.chunk, -1, 0, 0);
        }

        if (location.local.x == Chunk::Size - 1) {
            markExistingNeighborDirty(location.chunk, 1, 0, 0);
        }


        if (location.local.y == 0) {
            markExistingNeighborDirty(location.chunk, 0, -1, 0);
        }

        if (location.local.y == Chunk::Size - 1) {
            markExistingNeighborDirty(location.chunk, 0, 1, 0);
        }


        if (location.local.z == 0) {
            markExistingNeighborDirty(location.chunk, 0, 0, -1);
        }

        if (location.local.z == Chunk::Size - 1) {
            markExistingNeighborDirty(location.chunk, 0, 0, 1);
        }
    }



    std::vector<ChunkPosition> World::consumeDirtyChunks() {
        std::vector<ChunkPosition> positions{
            dirtyChunks_.begin(),
            dirtyChunks_.end()
        };


        dirtyChunks_.clear();


        return positions;
    }



    std::size_t World::chunkCount() const {
        return chunks_.size();
    }



    std::size_t World::dirtyChunkCount() const {
        return dirtyChunks_.size();
    }



    const std::map<ChunkPosition, Chunk, ChunkPositionLess>& World::chunks() const {
        return chunks_;
    }
}