#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "world/BlockType.h"
#include "world/Chunk.h"
#include "world/ChunkCoordinates.h"



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



    void testOrigin() {
        const ve::ChunkBlockLocation location =
            ve::worldToChunkLocation(ve::WorldBlockPosition{ 0, 0, 0 });


        require(location.chunk.x == 0, "Origin Chunk X must be zero");
        require(location.chunk.y == 0, "Origin Chunk Y must be zero");
        require(location.chunk.z == 0, "Origin Chunk Z must be zero");

        require(location.local.x == 0, "Origin Local X must be zero");
        require(location.local.y == 0, "Origin Local Y must be zero");
        require(location.local.z == 0, "Origin Local Z must be zero");


        std::cout << "[PASS] World origin conversion\n";
    }



    void testPositiveCoordinates() {
        const ve::ChunkBlockLocation location =
            ve::worldToChunkLocation(ve::WorldBlockPosition{ 19, 34, 5 });


        require(location.chunk.x == 1, "Positive Chunk X is incorrect");
        require(location.chunk.y == 2, "Positive Chunk Y is incorrect");
        require(location.chunk.z == 0, "Positive Chunk Z is incorrect");

        require(location.local.x == 3, "Positive Local X is incorrect");
        require(location.local.y == 2, "Positive Local Y is incorrect");
        require(location.local.z == 5, "Positive Local Z is incorrect");


        std::cout << "[PASS] Positive World coordinates\n";
    }



    void testNegativeCoordinates() {
        const ve::ChunkBlockLocation location =
            ve::worldToChunkLocation(ve::WorldBlockPosition{ -17, 34, -1 });


        require(location.chunk.x == -2, "Negative Chunk X is incorrect");
        require(location.chunk.y == 2, "Negative Chunk Y is incorrect");
        require(location.chunk.z == -1, "Negative Chunk Z is incorrect");

        require(location.local.x == 15, "Negative Local X is incorrect");
        require(location.local.y == 2, "Negative Local Y is incorrect");
        require(location.local.z == 15, "Negative Local Z is incorrect");


        std::cout << "[PASS] Negative World coordinates\n";
    }



    void testChunkBoundaries() {
        struct TestCase
        {
            int world;
            int chunk;
            int local;
        };


        constexpr std::array<TestCase, 10> cases{
            TestCase{ -33, -3, 15 },
            TestCase{ -17, -2, 15 },
            TestCase{ -16, -1, 0 },
            TestCase{ -1, -1, 15 },
            TestCase{ 0, 0, 0 },
            TestCase{ 1, 0, 1 },
            TestCase{ 15, 0, 15 },
            TestCase{ 16, 1, 0 },
            TestCase{ 31, 1, 15 },
            TestCase{ 32, 2, 0 }
        };


        for (const TestCase& test : cases) {
            const ve::ChunkBlockLocation location =
                ve::worldToChunkLocation(ve::WorldBlockPosition{ test.world, 0, 0 });


            require(location.chunk.x == test.chunk, "Chunk boundary mapping is incorrect");
            require(location.local.x == test.local, "Local boundary mapping is incorrect");
        }


        std::cout << "[PASS] Positive and negative Chunk boundaries\n";
    }



    void testRoundTrip() {
        for (int y = -33; y <= 33; ++y) {
            for (int z = -33; z <= 33; ++z) {
                for (int x = -33; x <= 33; ++x) {
                    const ve::WorldBlockPosition original{ x, y, z };

                    const ve::ChunkBlockLocation location =
                        ve::worldToChunkLocation(original);


                    require(
                        ve::Chunk::isInside(location.local.x, location.local.y, location.local.z),
                        "Converted Local coordinates must always be inside Chunk"
                    );


                    const ve::WorldBlockPosition restored =
                        ve::chunkToWorldPosition(location.chunk, location.local);


                    require(
                        restored.x == original.x &&
                        restored.y == original.y &&
                        restored.z == original.z,
                        "World to Chunk round-trip failed"
                    );
                }
            }
        }


        std::cout << "[PASS] World/Chunk coordinate round-trip\n";
    }



    void testIntegerLimits() {
        constexpr int minimum = std::numeric_limits<int>::min();
        constexpr int maximum = std::numeric_limits<int>::max();


        const std::array<ve::WorldBlockPosition, 2> positions{
            ve::WorldBlockPosition{ minimum, minimum, minimum },
            ve::WorldBlockPosition{ maximum, maximum, maximum }
        };


        for (const ve::WorldBlockPosition& position : positions) {
            const ve::ChunkBlockLocation location =
                ve::worldToChunkLocation(position);

            const ve::WorldBlockPosition restored =
                ve::chunkToWorldPosition(location.chunk, location.local);


            require(
                restored.x == position.x &&
                restored.y == position.y &&
                restored.z == position.z,
                "Integer limit coordinate round-trip failed"
            );
        }


        std::cout << "[PASS] Integer coordinate limits\n";
    }



    void testInvalidLocalCoordinates() {
        const ve::ChunkPosition chunk{ 0, 0, 0 };


        requireOutOfRange(
            [&chunk] {
                (void)ve::chunkToWorldPosition(chunk, ve::LocalBlockPosition{ -1, 0, 0 });
            },
            "Negative Local X must be rejected"
        );


        requireOutOfRange(
            [&chunk] {
                (void)ve::chunkToWorldPosition(chunk, ve::LocalBlockPosition{ 0, 16, 0 });
            },
            "Local Y=16 must be rejected"
        );


        requireOutOfRange(
            [&chunk] {
                (void)ve::chunkToWorldPosition(chunk, ve::LocalBlockPosition{ 0, 0, -1 });
            },
            "Negative Local Z must be rejected"
        );


        requireOutOfRange(
            [] {
                const ve::ChunkPosition huge{ std::numeric_limits<int>::max(), 0, 0 };

                (void)ve::chunkToWorldPosition(huge, ve::LocalBlockPosition{ 0, 0, 0 });
            },
            "World coordinate overflow must be rejected"
        );


        std::cout << "[PASS] Invalid local coordinates and overflow\n";
    }



    void testAdjacentChunkAccess() {
        ve::Chunk leftChunk{};
        ve::Chunk rightChunk{};


        const ve::ChunkBlockLocation leftLocation =
            ve::worldToChunkLocation(ve::WorldBlockPosition{ 15, 2, 5 });

        const ve::ChunkBlockLocation rightLocation =
            ve::worldToChunkLocation(ve::WorldBlockPosition{ 16, 2, 5 });


        require(leftLocation.chunk.x == 0, "Left block must belong to Chunk X=0");
        require(rightLocation.chunk.x == 1, "Right block must belong to Chunk X=1");


        leftChunk.setBlock(
            leftLocation.local.x,
            leftLocation.local.y,
            leftLocation.local.z,
            ve::BlockType::Grass
        );

        rightChunk.setBlock(
            rightLocation.local.x,
            rightLocation.local.y,
            rightLocation.local.z,
            ve::BlockType::Stone
        );


        require(
            leftChunk.blockAt(15, 2, 5) == ve::BlockType::Grass,
            "Left Chunk block access failed"
        );

        require(
            rightChunk.blockAt(0, 2, 5) == ve::BlockType::Stone,
            "Right Chunk block access failed"
        );


        std::cout << "[PASS] Adjacent Chunk block addressing\n";
    }
}



int main() {
    try {
        testOrigin();
        testPositiveCoordinates();
        testNegativeCoordinates();
        testChunkBoundaries();
        testRoundTrip();
        testIntegerLimits();
        testInvalidLocalCoordinates();
        testAdjacentChunkAccess();


        std::cout << "\nAll Chunk coordinate tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}