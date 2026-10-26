#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>

#include "assets/block/BlockRegistry.h"
#include "scene/voxel/VoxelWorld.h"
#include "voxel/chunk/Chunk.h"
#include "voxel/chunk/ChunkMeshBuilder.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void testFaceMapping() {
        using namespace ve;

        static_assert(BlockRegistry::material(BlockId::Grass, BlockFace::Top) == BlockMaterialKey::GrassTop);
        static_assert(BlockRegistry::material(BlockId::Grass, BlockFace::Bottom) == BlockMaterialKey::Dirt);
        static_assert(BlockRegistry::material(BlockId::Grass, BlockFace::Left) == BlockMaterialKey::GrassSide);
        static_assert(BlockRegistry::material(BlockId::Stone, BlockFace::Right) == BlockMaterialKey::Stone);

        Chunk chunk;
        chunk.setBlock(0, 0, 0, BlockId::Grass);

        const MaterialMeshes meshes = ChunkMeshBuilder::createMaterialMeshes(chunk);

        require(meshes.size() == 3, "Grass cube must have three material groups");

        require(
            meshes.at(BlockMaterialKey::GrassTop).triangleCount() == 2,
            "Grass top triangles mismatch"
        );

        require(
            meshes.at(BlockMaterialKey::GrassSide).triangleCount() == 8,
            "Grass side triangles mismatch"
        );

        require(
            meshes.at(BlockMaterialKey::Dirt).triangleCount() == 2,
            "Grass bottom triangles mismatch"
        );

        for (const auto& [key, mesh] : meshes) {
            (void)key;

            require(!mesh.vertices().empty(), "Material group must have vertices");

            for (const auto& vertex : mesh.vertices()) {
                require(
                    vertex.uv.x >= 0.0f && vertex.uv.x <= 1.0f &&
                    vertex.uv.y >= 0.0f && vertex.uv.y <= 1.0f,
                    "Face UV must remain normalized"
                );
            }
        }

        std::cout << "[PASS] Grass face Material mapping and UV\n";
    }


    void testCullingAcrossChunks() {
        using namespace ve;

        VoxelStorage world;

        const ChunkPosition a{ 0, 0, 0 };
        const ChunkPosition b{ 1, 0, 0 };

        world.setBlock(WorldBlockPosition{ 15, 0, 0 }, BlockId::Dirt);
        world.setBlock(WorldBlockPosition{ 16, 0, 0 }, BlockId::Dirt);

        const auto left = ChunkMeshBuilder::createMaterialMeshes(world, a);
        const auto right = ChunkMeshBuilder::createMaterialMeshes(world, b);

        require(
            left.at(BlockMaterialKey::Dirt).triangleCount() == 10,
            "Left boundary face must be hidden"
        );

        require(
            right.at(BlockMaterialKey::Dirt).triangleCount() == 10,
            "Right boundary face must be hidden"
        );

        std::cout << "[PASS] Cross-chunk hidden-face culling\n";
    }


    struct SubmeshState
    {
        std::uint64_t id = 0;
        std::uint64_t revision = 0;
    };


    std::map<ve::BlockMaterialKey, SubmeshState> snapshot( const ve::VoxelWorld& world ) {
        std::map<ve::BlockMaterialKey, SubmeshState> result;

        world.meshes().forEachMaterialMesh(
            [&]( const ve::ChunkPosition&, ve::BlockMaterialKey material,
                 const ve::MeshData&, std::uint64_t id, std::uint64_t revision ) {
                result.emplace(material, SubmeshState{ id, revision });
            }
        );

        return result;
    }


    void testRevisionAndRemoval() {
        using namespace ve;

        MeshIdGenerator ids;
        VoxelWorld world(ids);

        const ChunkPosition position{ 0, 0, 0 };

        world.blocks().setBlock(WorldBlockPosition{ 0, 0, 0 }, BlockId::Grass);

        require(
            world.rebuildDirtyMeshes() == 1,
            "First dirty Chunk must rebuild once"
        );

        const auto first = snapshot(world);

        require(first.size() == 3, "Grass material groups missing");

        world.blocks().setBlock(WorldBlockPosition{ 1, 0, 0 }, BlockId::Stone);

        require(
            world.rebuildDirtyMeshes() == 1,
            "Edited Chunk must rebuild once"
        );

        const auto second = snapshot(world);

        require(second.size() == 4, "Stone group must appear");

        require(
            second.at(BlockMaterialKey::GrassTop).id ==
            first.at(BlockMaterialKey::GrassTop).id,
            "Existing Submesh ID must remain stable"
        );

        require(
            second.at(BlockMaterialKey::GrassTop).revision ==
            first.at(BlockMaterialKey::GrassTop).revision + 1,
            "Revision must advance on rebuild"
        );

        require(world.removeChunk(position), "Chunk removal failed");

        require(
            snapshot(world).empty(),
            "Removed Chunk must release CPU Submeshes"
        );

        std::cout << "[PASS] Material Submesh ID, Revision and removal\n";
    }
}


int main() {
    try {
        testFaceMapping();
        testCullingAcrossChunks();
        testRevisionAndRemoval();

        std::cout << "All Voxel Material Submesh tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}