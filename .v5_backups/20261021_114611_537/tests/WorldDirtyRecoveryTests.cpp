#include <cstddef>
#include <exception>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include "assets/mesh/MeshData.h"
#include "world/meshing/ChunkMeshFactory.h"

#include "world/BlockType.h"
#include "world/Chunk.h"
#include "world/ChunkCoordinates.h"
#include "world/World.h"



namespace
{
    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    bool samePosition( const ve::ChunkPosition& a, const ve::ChunkPosition& b ) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }



    void testSuccessfulProcessing() {
        ve::World world{};

        const ve::ChunkPosition position{ 0, 0, 0 };

        world.getOrCreateChunk(position);


        std::size_t processed = 0;



        world.processDirtyChunks([&]( const ve::ChunkPosition& current ) {
            require(samePosition(current, position), "Unexpected Chunk processed");

            require(world.dirtyChunkCount() == 1, "Dirty state must exist during processing");

            ++processed;
        });



        require(processed == 1, "Chunk must be processed exactly once");
        require(world.dirtyChunkCount() == 0, "Successful Chunk must become clean");


        std::cout << "[PASS] Successful work clears Dirty state\n";
    }



    void testEmptyProcessing() {
        ve::World world{};

        int calls = 0;



        world.processDirtyChunks([&]( const ve::ChunkPosition& position ) {
            (void)position;
            ++calls;
        });



        require(calls == 0, "Empty Dirty set must not invoke callback");
        require(world.dirtyChunkCount() == 0, "Empty World must remain clean");


        std::cout << "[PASS] Empty Dirty queue processing\n";
    }



    void testFailurePreservesRemainingWork() {
        ve::World world{};



        // Three Nonadjacent Chunks

        world.getOrCreateChunk(ve::ChunkPosition{ -2, 0, 0 });
        world.getOrCreateChunk(ve::ChunkPosition{ 0, 0, 0 });
        world.getOrCreateChunk(ve::ChunkPosition{ 2, 0, 0 });


        require(world.dirtyChunkCount() == 3, "Expected three Dirty Chunks");


        std::vector<ve::ChunkPosition> firstAttempt;

        bool failed = false;



        try {
            world.processDirtyChunks([&]( const ve::ChunkPosition& position ) {
                firstAttempt.push_back(position);


                if (position.x == 0) {
                    throw std::runtime_error("Simulated Chunk Mesh failure");
                }
            });
        }
        catch (const std::runtime_error&) {
            failed = true;
        }



        require(failed, "Simulated failure must propagate to caller");

        require(firstAttempt.size() == 2, "Processing must stop at the failing Chunk");
        require(firstAttempt[0].x == -2, "First Chunk must process successfully");
        require(firstAttempt[1].x == 0, "Second Chunk must fail");

        require(world.dirtyChunkCount() == 2, "Failed and unprocessed Chunks must remain Dirty");



        // Retry After the Simulated Failure Is Resolved

        std::vector<ve::ChunkPosition> secondAttempt;


        world.processDirtyChunks([&]( const ve::ChunkPosition& position ) {
            secondAttempt.push_back(position);
        });



        require(secondAttempt.size() == 2, "Retry must process exactly two Chunks");

        require(secondAttempt[0].x == 0, "Failed Chunk must be retried");
        require(secondAttempt[1].x == 2, "Unprocessed Chunk must be retried");

        require(world.dirtyChunkCount() == 0, "All Chunks must be clean after retry");


        std::cout << "[PASS] Failure preserves pending work for retry\n";
    }



    void testActualMeshRebuildRetry() {
        ve::World world{};

        const ve::ChunkPosition leftPosition{ -1, 0, 0 };
        const ve::ChunkPosition rightPosition{ 0, 0, 0 };

        const ve::WorldBlockPosition leftBlock{ -1, 5, 5 };
        const ve::WorldBlockPosition rightBlock{ 0, 5, 5 };



        // Start with Two Touching Stone Blocks

        world.setBlock(leftBlock, ve::BlockType::Stone);
        world.setBlock(rightBlock, ve::BlockType::Stone);



        std::map<ve::ChunkPosition, ve::MeshData, ve::ChunkPositionLess> meshes;

        meshes.emplace(leftPosition, ve::ChunkMeshFactory::create(world, leftPosition));
        meshes.emplace(rightPosition, ve::ChunkMeshFactory::create(world, rightPosition));


        world.consumeDirtyChunks();



        // Each Block Initially Has Five Exposed Faces

        require(meshes.at(leftPosition).vertexCount() == 20, "Initial left Mesh is incorrect");
        require(meshes.at(rightPosition).vertexCount() == 20, "Initial right Mesh is incorrect");



        // Delete the Right Stone Block

        world.setBlock(rightBlock, ve::BlockType::Air);

        require(world.dirtyChunkCount() == 2, "Both boundary Chunks must become Dirty");



        bool injectFailure = true;
        bool caught = false;



        // First Attempt: Left Mesh Succeeds, Right Mesh Fails

        try {
            world.processDirtyChunks([&]( const ve::ChunkPosition& position ) {
                if (samePosition(position, rightPosition) && injectFailure) {
                    injectFailure = false;

                    throw std::runtime_error("Simulated Mesh generation failure");
                }


                meshes.at(position) = ve::ChunkMeshFactory::create(world, position);
            });
        }
        catch (const std::runtime_error&) {
            caught = true;
        }



        require(caught, "Mesh rebuild failure must be detected");

        require(meshes.at(leftPosition).vertexCount() == 24, "Left Mesh must regain exposed face");
        require(meshes.at(rightPosition).vertexCount() == 20, "Failed right Mesh must remain unchanged");

        require(world.dirtyChunkCount() == 1, "Only the failed right Chunk must remain Dirty");



        // Second Attempt: Rebuild the Remaining Dirty Chunk

        world.processDirtyChunks([&]( const ve::ChunkPosition& position ) {
            meshes.at(position) = ve::ChunkMeshFactory::create(world, position);
        });



        require(meshes.at(leftPosition).vertexCount() == 24, "Successful Mesh must not be rebuilt incorrectly");
        require(meshes.at(rightPosition).vertexCount() == 0, "Deleted block must produce empty Mesh");

        require(world.dirtyChunkCount() == 0, "Retry must clear all remaining Dirty states");


        std::cout << "[PASS] Actual Chunk Mesh rebuild failure and recovery\n";
    }
}



int main() {
    try {
        testSuccessfulProcessing();
        testEmptyProcessing();
        testFailurePreservesRemainingWork();
        testActualMeshRebuildRetry();


        std::cout << "\nAll World Dirty Recovery tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}