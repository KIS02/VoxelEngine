#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "voxel/block/BlockId.h"
#include "voxel/chunk/Chunk.h"



namespace
{
    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    template <typename Callback>
    void requireOutOfRange( Callback callback, const char* message ) {
        bool caught = false;


        try {
            callback();
        }
        catch (const std::out_of_range&) {
            caught = true;
        }


        require(caught, message);
    }



    void testChunkDimensions() {
        require(ve::Chunk::Size == 16, "Chunk size must be 16");
        require(ve::Chunk::BlockCount == 4096, "Chunk must contain 4096 blocks");

        require(sizeof(ve::BlockId) == 1, "BlockId must occupy one byte");

        std::cout << "[PASS] Chunk dimensions and block storage type\n";
    }



    void testDefaultBlocks() {
        const ve::Chunk chunk{};


        for (int y = 0; y < ve::Chunk::Size; ++y) {
            for (int z = 0; z < ve::Chunk::Size; ++z) {
                for (int x = 0; x < ve::Chunk::Size; ++x) {
                    require(
                        chunk.blockAt(x, y, z) == ve::BlockId::Air,
                        "New Chunk must contain only Air"
                    );
                }
            }
        }


        require(chunk.solidBlockCount() == 0, "New Chunk must have zero solid blocks");

        std::cout << "[PASS] Chunk default initialization\n";
    }



    void testBlockAccess() {
        ve::Chunk chunk{};


        chunk.setBlock(0, 0, 0, ve::BlockId::Grass);
        chunk.setBlock(1, 0, 0, ve::BlockId::Dirt);
        chunk.setBlock(0, 0, 1, ve::BlockId::Stone);
        chunk.setBlock(0, 1, 0, ve::BlockId::Grass);
        chunk.setBlock(15, 15, 15, ve::BlockId::Stone);


        require(chunk.blockAt(0, 0, 0) == ve::BlockId::Grass, "Incorrect origin block");
        require(chunk.blockAt(1, 0, 0) == ve::BlockId::Dirt, "Incorrect X neighbor block");
        require(chunk.blockAt(0, 0, 1) == ve::BlockId::Stone, "Incorrect Z neighbor block");
        require(chunk.blockAt(0, 1, 0) == ve::BlockId::Grass, "Incorrect Y neighbor block");
        require(chunk.blockAt(15, 15, 15) == ve::BlockId::Stone, "Incorrect last block");


        require(chunk.blockAt(2, 0, 0) == ve::BlockId::Air, "Unmodified block must remain Air");

        require(chunk.solidBlockCount() == 5, "Chunk must contain five solid blocks");


        std::cout << "[PASS] Chunk block access and coordinate mapping\n";
    }



    void testBlockReplacement() {
        ve::Chunk chunk{};


        chunk.setBlock(5, 3, 7, ve::BlockId::Dirt);
        require(chunk.solidBlockCount() == 1, "Solid block count must be one");


        chunk.setBlock(5, 3, 7, ve::BlockId::Stone);

        require(chunk.blockAt(5, 3, 7) == ve::BlockId::Stone, "Block replacement failed");
        require(chunk.solidBlockCount() == 1, "Replacing a solid block must not change count");


        chunk.setBlock(5, 3, 7, ve::BlockId::Air);

        require(chunk.blockAt(5, 3, 7) == ve::BlockId::Air, "Block removal failed");
        require(chunk.solidBlockCount() == 0, "Removing a solid block must update count");


        std::cout << "[PASS] Chunk block replacement\n";
    }



    void testFill() {
        ve::Chunk chunk{};


        chunk.fill(ve::BlockId::Dirt);

        require(
            chunk.solidBlockCount() == ve::Chunk::BlockCount,
            "Filled Chunk must contain 4096 solid blocks"
        );


        require(chunk.blockAt(0, 0, 0) == ve::BlockId::Dirt, "Fill failed at origin");
        require(chunk.blockAt(15, 15, 15) == ve::BlockId::Dirt, "Fill failed at last block");


        chunk.fill(ve::BlockId::Air);

        require(chunk.solidBlockCount() == 0, "Cleared Chunk must contain zero solid blocks");


        std::cout << "[PASS] Chunk fill and clear\n";
    }



    void testCoordinateValidation() {
        require(ve::Chunk::isInside(0, 0, 0), "Origin must be inside Chunk");
        require(ve::Chunk::isInside(15, 15, 15), "Last coordinate must be inside Chunk");

        require(!ve::Chunk::isInside(-1, 0, 0), "Negative X must be outside");
        require(!ve::Chunk::isInside(0, -1, 0), "Negative Y must be outside");
        require(!ve::Chunk::isInside(0, 0, -1), "Negative Z must be outside");

        require(!ve::Chunk::isInside(16, 0, 0), "X=16 must be outside");
        require(!ve::Chunk::isInside(0, 16, 0), "Y=16 must be outside");
        require(!ve::Chunk::isInside(0, 0, 16), "Z=16 must be outside");


        ve::Chunk chunk{};


        requireOutOfRange(
            [&chunk] { (void)chunk.blockAt(-1, 0, 0); },
            "Reading negative X must throw"
        );

        requireOutOfRange(
            [&chunk] { (void)chunk.blockAt(16, 0, 0); },
            "Reading X=16 must throw"
        );

        requireOutOfRange(
            [&chunk] { chunk.setBlock(0, 16, 0, ve::BlockId::Stone); },
            "Writing Y=16 must throw"
        );

        requireOutOfRange(
            [&chunk] { chunk.setBlock(0, 0, -1, ve::BlockId::Stone); },
            "Writing negative Z must throw"
        );


        std::cout << "[PASS] Chunk coordinate validation\n";
    }



    void testBlockProperties() {
        require(!ve::isSolid(ve::BlockId::Air), "Air must not be solid");
        require(ve::isSolid(ve::BlockId::Grass), "Grass must be solid");
        require(ve::isSolid(ve::BlockId::Dirt), "Dirt must be solid");
        require(ve::isSolid(ve::BlockId::Stone), "Stone must be solid");


        std::cout << "[PASS] Block properties\n";
    }
}



int main() {
    try {
        testChunkDimensions();
        testDefaultBlocks();
        testBlockAccess();
        testBlockReplacement();
        testFill();
        testCoordinateValidation();
        testBlockProperties();


        std::cout << "\nAll Chunk tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}