#include <exception>
#include <iostream>
#include <stdexcept>

#include "voxel/generation/TerrainGenerator.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void testConfigurationOwnership() {
        ve::TerrainGenerationConfig settings{ 987654321ULL, 5, 1 };
        const ve::TerrainGenerator generator(settings);

        settings.seed = 42;

        require(generator.config().seed == 987654321ULL, "Generator must retain its own config copy");
        require(generator.config().surfaceY == 5, "Surface height must be saved");
        require(generator.config().dirtDepth == 1, "Dirt depth must be saved");

        std::cout << "[PASS] Configuration ownership\n";
    }


    void testSurfaceLayers() {
        const ve::TerrainGenerator generator(ve::TerrainGenerationConfig{ 100, 3, 2 });
        const ve::Chunk chunk = generator.generateFlat(ve::ChunkPosition{ 0, 0, 0 });

        require(chunk.blockAt(5, 4, 5) == ve::BlockId::Air, "Above surface must be Air");
        require(chunk.blockAt(5, 3, 5) == ve::BlockId::Grass, "Surface must be Grass");
        require(chunk.blockAt(5, 2, 5) == ve::BlockId::Dirt, "First layer must be Dirt");
        require(chunk.blockAt(5, 1, 5) == ve::BlockId::Dirt, "Second layer must be Dirt");
        require(chunk.blockAt(5, 0, 5) == ve::BlockId::Stone, "Deep layer must be Stone");

        std::cout << "[PASS] Surface layers\n";
    }


    void testVerticalChunks() {
        const ve::TerrainGenerator generator(ve::TerrainGenerationConfig{ 100, 3, 2 });
        const ve::Chunk below = generator.generateFlat(ve::ChunkPosition{ 0, -1, 0 });
        const ve::Chunk above = generator.generateFlat(ve::ChunkPosition{ 0, 1, 0 });

        require(below.solidBlockCount() == ve::Chunk::BlockCount,
                "Below-surface Chunk must be full");

        require(above.solidBlockCount() == 0,
                "Above-surface Chunk must be empty");

        std::cout << "[PASS] Vertical Chunks\n";
    }


    void testHorizontalBoundary() {
        const ve::TerrainGenerator generator(ve::TerrainGenerationConfig{ 100, 3, 2 });
        const ve::Chunk left = generator.generateFlat(ve::ChunkPosition{ -1, 0, 0 });
        const ve::Chunk right = generator.generateFlat(ve::ChunkPosition{ 0, 0, 0 });

        for (int y = 0; y < ve::Chunk::Size; ++y)
            require(left.blockAt(15, y, 5) == right.blockAt(0, y, 5),
                    "Flat terrain must match across Chunk boundary");

        std::cout << "[PASS] Horizontal Chunk boundary\n";
    }


    void testDirtDepth() {
        const ve::TerrainGenerator generator(ve::TerrainGenerationConfig{ 100, 5, 1 });
        const ve::Chunk chunk = generator.generateFlat(ve::ChunkPosition{ 0, 0, 0 });

        require(chunk.blockAt(5, 5, 5) == ve::BlockId::Grass, "Surface must be Grass");
        require(chunk.blockAt(5, 4, 5) == ve::BlockId::Dirt, "First lower layer must be Dirt");
        require(chunk.blockAt(5, 3, 5) == ve::BlockId::Stone, "Second lower layer must be Stone");

        std::cout << "[PASS] Configurable Dirt depth\n";
    }


    void testSameSeedDeterminism() {
        const ve::TerrainGenerator first(ve::TerrainGenerationConfig{ 12345, 3, 2 });
        const ve::TerrainGenerator second(ve::TerrainGenerationConfig{ 12345, 3, 2 });

        const ve::Chunk a = first.generateFlat(ve::ChunkPosition{ -2, 0, 3 });
        const ve::Chunk b = second.generateFlat(ve::ChunkPosition{ -2, 0, 3 });

        for (int y = 0; y < ve::Chunk::Size; ++y) {
            for (int z = 0; z < ve::Chunk::Size; ++z) {
                for (int x = 0; x < ve::Chunk::Size; ++x)
                    require(a.blockAt(x, y, z) == b.blockAt(x, y, z),
                            "Identical config must produce identical Blocks");
            }
        }

        std::cout << "[PASS] Same inputs, same Chunk\n";
    }


    void testSeedNotYetUsed() {
        const ve::TerrainGenerator first(ve::TerrainGenerationConfig{ 1, 3, 2 });
        const ve::TerrainGenerator second(ve::TerrainGenerationConfig{ 2, 3, 2 });

        const ve::Chunk a = first.generateFlat(ve::ChunkPosition{ 0, 0, 0 });
        const ve::Chunk b = second.generateFlat(ve::ChunkPosition{ 0, 0, 0 });

        for (int y = 0; y < ve::Chunk::Size; ++y) {
            for (int z = 0; z < ve::Chunk::Size; ++z) {
                for (int x = 0; x < ve::Chunk::Size; ++x)
                    require(a.blockAt(x, y, z) == b.blockAt(x, y, z),
                            "Flat generator must currently ignore Seed");
            }
        }

        std::cout << "[PASS] Flat generation intentionally ignores Seed\n";
    }


    void testInvalidConfiguration() {
        bool thrown = false;

        try {
            const ve::TerrainGenerator invalid(ve::TerrainGenerationConfig{ 1, 3, -1 });
        } catch (const std::invalid_argument&) {
            thrown = true;
        }

        require(thrown, "Negative dirtDepth must be rejected");

        std::cout << "[PASS] Invalid configuration\n";
    }
}


int main() {
    try {
        testConfigurationOwnership();
        testSurfaceLayers();
        testVerticalChunks();
        testHorizontalBoundary();
        testDirtDepth();
        testSameSeedDeterminism();
        testSeedNotYetUsed();
        testInvalidConfiguration();

        std::cout << "All TerrainGenerator unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}