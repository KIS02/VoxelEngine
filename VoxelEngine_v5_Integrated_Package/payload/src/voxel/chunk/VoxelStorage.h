#pragma once

#include <cstddef>
#include <map>
#include <set>
#include <vector>

#include "voxel/block/BlockId.h"
#include "voxel/chunk/Chunk.h"
#include "voxel/chunk/ChunkCoordinates.h"



namespace ve
{
    struct ChunkPositionLess
    {
        constexpr bool operator()( const ChunkPosition& a, const ChunkPosition& b ) const {
            if (a.x != b.x) return a.x < b.x;
            if (a.y != b.y) return a.y < b.y;
            return a.z < b.z;
        }
    };



    class VoxelStorage {
        private:
            std::map<ChunkPosition, Chunk, ChunkPositionLess> chunks_;
            std::set<ChunkPosition, ChunkPositionLess> dirtyChunks_;


        public:
            VoxelStorage() = default;


            Chunk& getOrCreateChunk( const ChunkPosition& position );

            Chunk* findChunk( const ChunkPosition& position );
            const Chunk* findChunk( const ChunkPosition& position ) const;

            bool removeChunk( const ChunkPosition& position );

            BlockId blockAt( const WorldBlockPosition& position ) const;

            void setBlock( const WorldBlockPosition& position, BlockId type );

            void markChunkDirty( const ChunkPosition& position );

            std::vector<ChunkPosition> consumeDirtyChunks();

            template <typename Callback>
            void processDirtyChunks( Callback&& callback ) {
                const std::vector<ChunkPosition> pending{
                    dirtyChunks_.begin(), dirtyChunks_.end()
                };


                for (const ChunkPosition& position : pending) {
                    callback(position);

                    dirtyChunks_.erase(position);
                }
            }

            std::size_t chunkCount() const;
            std::size_t dirtyChunkCount() const;

            const std::map<ChunkPosition, Chunk, ChunkPositionLess>& chunks() const;


        private:
            void markExistingNeighborDirty( const ChunkPosition& position, int dx, int dy, int dz );
    };
}