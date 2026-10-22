#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "assets/mesh/MeshIdGenerator.h"
#include "scene/voxel/VoxelWorld.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    struct MeshSnapshot
    {
        bool found = false;
        std::uint64_t id = 0;
        std::uint64_t revision = 0;
        std::size_t triangleCount = 0;
    };


    MeshSnapshot snapshot( const ve::VoxelWorld& world, const ve::ChunkPosition& target ) {
        MeshSnapshot result{};

        world.meshes().forEachMesh([&]( const ve::ChunkPosition& position, const ve::MeshData& mesh,
                                        std::uint64_t id, std::uint64_t revision ) {
            if (position.x != target.x || position.y != target.y || position.z != target.z)
                return;

            result.found = true;
            result.id = id;
            result.revision = revision;
            result.triangleCount = mesh.triangleCount();
        });

        return result;
    }


    void testInsertionAndOwnership() {
        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);
        const ve::ChunkPosition position{ 0, 0, 0 };

        ve::Chunk generated{};
        generated.setBlock(2, 3, 4, ve::BlockId::Grass);

        world.blocks().setChunk(position, std::move(generated));

        require(world.blocks().chunkCount() == 1,
                "Chunk must be owned by VoxelStorage");

        require(world.blocks().blockAt(ve::WorldBlockPosition{ 2, 3, 4 }) == ve::BlockId::Grass,
                "Stored Chunk must preserve block content");

        require(world.blocks().dirtyChunkCount() == 1,
                "New Chunk must become Dirty");

        require(world.rebuildDirtyMeshes() == 1,
                "New Chunk must be rebuilt once");

        const MeshSnapshot built = snapshot(world, position);

        require(built.found && built.revision == 1 && built.id != 0,
                "First Mesh must exist");

        require(built.triangleCount == 12,
                "Single isolated block must have twelve triangles");

        require(world.blocks().dirtyChunkCount() == 0,
                "Rebuild must consume Dirty state");

        std::cout << "[PASS] Insertion and ownership\n";
    }


    void testNeighborRevisionAndReplacement() {
        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);

        const ve::ChunkPosition left{ -1, 0, 0 };
        const ve::ChunkPosition right{ 0, 0, 0 };

        ve::Chunk leftChunk{};
        leftChunk.setBlock(15, 1, 1, ve::BlockId::Stone);

        world.blocks().setChunk(left, std::move(leftChunk));

        require(world.rebuildDirtyMeshes() == 1,
                "Left initial rebuild failed");

        const MeshSnapshot firstLeft = snapshot(world, left);

        require(firstLeft.triangleCount == 12,
                "Initial isolated block should expose six faces");


        ve::Chunk rightChunk{};
        rightChunk.setBlock(0, 1, 1, ve::BlockId::Stone);

        world.blocks().setChunk(right, std::move(rightChunk));

        require(world.blocks().dirtyChunkCount() == 2,
                "New neighbor must Dirty itself and left");

        require(world.rebuildDirtyMeshes() == 2,
                "Both adjacent Meshes must rebuild");

        const MeshSnapshot secondLeft = snapshot(world, left);
        const MeshSnapshot firstRight = snapshot(world, right);

        require(secondLeft.id == firstLeft.id,
                "Left Mesh ID must remain stable");

        require(secondLeft.revision == firstLeft.revision + 1,
                "Left revision must increase");

        require(secondLeft.triangleCount == 10 && firstRight.triangleCount == 10,
                "Shared boundary must hide facing sides");


        ve::Chunk replacement{};
        world.blocks().setChunk(right, std::move(replacement));

        require(world.blocks().dirtyChunkCount() == 2,
                "Replacement must Dirty both neighbors");

        require(world.rebuildDirtyMeshes() == 2,
                "Replacement must rebuild both Meshes");

        const MeshSnapshot thirdLeft = snapshot(world, left);
        const MeshSnapshot secondRight = snapshot(world, right);

        require(thirdLeft.id == firstLeft.id &&
                thirdLeft.revision == secondLeft.revision + 1,
                "Left revision and ID must remain consistent");

        require(thirdLeft.triangleCount == 12,
                "Removed neighbor block must expose face again");

        require(secondRight.id == firstRight.id &&
                secondRight.revision == firstRight.revision + 1,
                "Right replacement must preserve Mesh ID and bump revision");

        require(secondRight.triangleCount == 0,
                "Empty replacement Chunk must produce empty Mesh");

        std::cout << "[PASS] Neighbor Dirty and Mesh revisions\n";
    }
}


int main() {
    try {
        testInsertionAndOwnership();
        testNeighborRevisionAndReplacement();

        std::cout << "All WorldChunkInsertion unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}