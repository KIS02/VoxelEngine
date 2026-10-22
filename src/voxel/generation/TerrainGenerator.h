#pragma once

#include "voxel/chunk/Chunk.h"
#include "voxel/chunk/ChunkCoordinates.h"
#include "voxel/generation/TerrainGenerationConfig.h"


namespace ve
{
    class TerrainGenerator {
        private:
            TerrainGenerationConfig config_{};


        public:
            explicit TerrainGenerator( TerrainGenerationConfig config );


            const TerrainGenerationConfig& config() const;

            Chunk generateFlat( const ChunkPosition& position ) const;

            Chunk generate( const ChunkPosition& position ) const;
    };
}