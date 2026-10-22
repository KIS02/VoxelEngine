#include "voxel/generation/TerrainGenerator.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

#include "voxel/generation/FractalNoise2D.h"


namespace ve
{
    namespace
    {
        void fillColumn( Chunk& chunk, const ChunkPosition& position, int localX, int localZ,
                         int surfaceY, int dirtDepth ) {
            for (int y = 0; y < Chunk::Size; ++y) {
                const LocalBlockPosition local{ localX, y, localZ };
                const WorldBlockPosition world = chunkToWorldPosition(position, local);

                if (world.y > surfaceY)
                    continue;

                const std::int64_t depth = static_cast<std::int64_t>(surfaceY) - world.y;
                BlockId type = BlockId::Stone;

                if (depth == 0)
                    type = BlockId::Grass;
                else if (depth <= dirtDepth)
                    type = BlockId::Dirt;

                chunk.setBlock(localX, y, localZ, type);
            }
        }
    }


    TerrainGenerator::TerrainGenerator( TerrainGenerationConfig config ) : config_(config) {
        if (config_.dirtDepth < 0)
            throw std::invalid_argument("Terrain dirtDepth cannot be negative");

        if (!std::isfinite(config_.heightAmplitude) || config_.heightAmplitude < 0.0)
            throw std::invalid_argument("Terrain heightAmplitude must be finite and non-negative");
    }


    const TerrainGenerationConfig& TerrainGenerator::config() const {
        return config_;
    }


    Chunk TerrainGenerator::generateFlat( const ChunkPosition& position ) const {
        Chunk chunk{};

        for (int z = 0; z < Chunk::Size; ++z) {
            for (int x = 0; x < Chunk::Size; ++x)
                fillColumn(chunk, position, x, z, config_.surfaceY, config_.dirtDepth);
        }

        return chunk;
    }


    Chunk TerrainGenerator::generate( const ChunkPosition& position ) const {
        Chunk chunk{};

        for (int z = 0; z < Chunk::Size; ++z) {
            for (int x = 0; x < Chunk::Size; ++x) {
                const LocalBlockPosition local{ x, 0, z };
                const WorldBlockPosition world = chunkToWorldPosition(position, local);

                const double noise = fractalNoise2D(
                    config_.seed, static_cast<double>(world.x), static_cast<double>(world.z), config_.noise
                );

                const double rawHeight = static_cast<double>(config_.surfaceY) +
                                         (noise - 0.5) * config_.heightAmplitude;
                const double flooredHeight = std::floor(rawHeight);

                if (!std::isfinite(flooredHeight) ||
                    flooredHeight < static_cast<double>(std::numeric_limits<int>::min()) ||
                    flooredHeight > static_cast<double>(std::numeric_limits<int>::max()))
                    throw std::out_of_range("Terrain height exceeds integer range");

                const int surfaceY = static_cast<int>(flooredHeight);

                fillColumn(chunk, position, x, z, surfaceY, config_.dirtDepth);
            }
        }

        return chunk;
    }
}