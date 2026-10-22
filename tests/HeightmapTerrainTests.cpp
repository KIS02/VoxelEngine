#include <cmath>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "voxel/generation/FractalNoise2D.h"
#include "voxel/generation/TerrainGenerator.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void testZeroAmplitudeMatchesFlat() {
        const ve::TerrainGenerationConfig config{ 1234, 5, 2, 0.0 };
        const ve::TerrainGenerator generator(config);
        const ve::ChunkPosition positions[]{ { 0, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 } };

        for (const ve::ChunkPosition& position : positions) {
            const ve::Chunk flat = generator.generateFlat(position);
            const ve::Chunk terrain = generator.generate(position);

            for (int y = 0; y < ve::Chunk::Size; ++y) {
                for (int z = 0; z < ve::Chunk::Size; ++z) {
                    for (int x = 0; x < ve::Chunk::Size; ++x)
                        require(flat.blockAt(x, y, z) == terrain.blockAt(x, y, z),
                                "Zero amplitude must match flat terrain");
                }
            }
        }

        std::cout << "[PASS] Zero amplitude matches flat terrain\n";
    }


    void testWorldCoordinatesAndLayers() {
        ve::TerrainGenerationConfig config{ 987654321, 7, 2, 10.0 };
        config.noise = ve::FractalNoiseConfig{ 3, 0.08, 2.0, 0.5 };

        const ve::TerrainGenerator generator(config);
        const ve::ChunkPosition positions[]{ { -1, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 } };

        int minHeight = 100;
        int maxHeight = -100;

        for (const ve::ChunkPosition& position : positions) {
            const ve::Chunk chunk = generator.generate(position);

            for (int z : { 0, 7, 15 }) {
                for (int x = 0; x < ve::Chunk::Size; ++x) {
                    const ve::WorldBlockPosition world = ve::chunkToWorldPosition(
                        position, ve::LocalBlockPosition{ x, 0, z }
                    );

                    const double noise = ve::fractalNoise2D(config.seed, world.x, world.z, config.noise);
                    const int surfaceY = static_cast<int>(std::floor(
                        config.surfaceY + (noise - 0.5) * config.heightAmplitude
                    ));

                    if (surfaceY < minHeight)
                        minHeight = surfaceY;

                    if (surfaceY > maxHeight)
                        maxHeight = surfaceY;

                    for (int y = 0; y < ve::Chunk::Size; ++y) {
                        ve::BlockId expected = ve::BlockId::Stone;

                        if (y > surfaceY)
                            expected = ve::BlockId::Air;
                        else if (y == surfaceY)
                            expected = ve::BlockId::Grass;
                        else if (surfaceY - y <= config.dirtDepth)
                            expected = ve::BlockId::Dirt;

                        require(chunk.blockAt(x, y, z) == expected,
                                "World-coordinate height or surface layer is incorrect");
                    }
                }
            }
        }

        require(minHeight < maxHeight, "Terrain should vary in height");

        std::cout << "[PASS] World X/Z coordinates, negative Chunks and layers\n";
    }


    void testDeterminismAndSeedVariation() {
        const ve::TerrainGenerationConfig aConfig{ 12, 8, 2, 16.0 };
        const ve::TerrainGenerationConfig bConfig{ 13, 8, 2, 16.0 };

        const ve::TerrainGenerator a(aConfig);
        const ve::TerrainGenerator identical(aConfig);
        const ve::TerrainGenerator b(bConfig);

        const ve::Chunk one = a.generate(ve::ChunkPosition{ 0, 0, 0 });
        const ve::Chunk repeat = a.generate(ve::ChunkPosition{ 0, 0, 0 });
        const ve::Chunk same = identical.generate(ve::ChunkPosition{ 0, 0, 0 });
        const ve::Chunk different = b.generate(ve::ChunkPosition{ 0, 0, 0 });

        bool changed = false;

        for (int y = 0; y < ve::Chunk::Size; ++y) {
            for (int z = 0; z < ve::Chunk::Size; ++z) {
                for (int x = 0; x < ve::Chunk::Size; ++x) {
                    require(one.blockAt(x, y, z) == repeat.blockAt(x, y, z),
                            "Repeated generation must be deterministic");

                    require(one.blockAt(x, y, z) == same.blockAt(x, y, z),
                            "Independent generators with same config must agree");

                    if (one.blockAt(x, y, z) != different.blockAt(x, y, z))
                        changed = true;
                }
            }
        }

        require(changed, "Different seeds should change this sample terrain");

        std::cout << "[PASS] Determinism and different seeds\n";
    }


    void testVerticalChunks() {
        const ve::TerrainGenerator generator(ve::TerrainGenerationConfig{ 12, 3, 2, 6.0 });

        const ve::Chunk below = generator.generate(ve::ChunkPosition{ 0, -2, 0 });
        const ve::Chunk above = generator.generate(ve::ChunkPosition{ 0, 1, 0 });

        require(below.solidBlockCount() == ve::Chunk::BlockCount,
                "Deep Chunk must be full");

        require(above.solidBlockCount() == 0,
                "High Chunk must be empty");

        std::cout << "[PASS] Vertical Chunks\n";
    }


    void testInvalidHeightAmplitude() {
        bool invalidNegative = false;
        bool invalidInfinite = false;

        try {
            const ve::TerrainGenerator invalid(ve::TerrainGenerationConfig{ 12, 3, 2, -1.0 });
        } catch (const std::invalid_argument&) {
            invalidNegative = true;
        }

        try {
            const ve::TerrainGenerator invalid(ve::TerrainGenerationConfig{
                12, 3, 2, std::numeric_limits<double>::infinity()
            });
        } catch (const std::invalid_argument&) {
            invalidInfinite = true;
        }

        require(invalidNegative && invalidInfinite, "Invalid amplitude must be rejected");

        std::cout << "[PASS] Invalid amplitude\n";
    }
}


int main() {
    try {
        testZeroAmplitudeMatchesFlat();
        testWorldCoordinatesAndLayers();
        testDeterminismAndSeedVariation();
        testVerticalChunks();
        testInvalidHeightAmplitude();

        std::cout << "All HeightmapTerrain unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}