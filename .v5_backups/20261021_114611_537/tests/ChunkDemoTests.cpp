#include <exception>
#include <iostream>
#include <stdexcept>

#include "demo/ChunkDemoFactory.h"

#include "assets/mesh/MeshData.h"
#include "world/meshing/ChunkMeshFactory.h"

#include "world/BlockType.h"
#include "world/Chunk.h"



namespace
{
    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    void testTerrainHeights() {
        const ve::Chunk chunk = ve::createDemoChunk();



        for (int z = 0; z < ve::Chunk::Size; ++z) {
            for (int x = 0; x < ve::Chunk::Size; ++x) {
                const int height = 2 + ((x / 4 + z / 4) % 3);



                for (int y = 0; y < ve::Chunk::Size; ++y) {
                    const ve::BlockType type = chunk.blockAt(x, y, z);


                    if (y >= height) {
                        require(type == ve::BlockType::Air, "Above terrain must be Air");
                    }
                    else if (y == height - 1) {
                        require(type == ve::BlockType::Grass, "Top block must be Grass");
                    }
                    else if (y == height - 2) {
                        require(type == ve::BlockType::Dirt, "Block below Grass must be Dirt");
                    }
                    else {
                        require(type == ve::BlockType::Stone, "Deep terrain must be Stone");
                    }
                }
            }
        }


        std::cout << "[PASS] Demo terrain block heights and types\n";
    }



    void testChunkMeshGeneration() {
        const ve::Chunk chunk = ve::createDemoChunk();

        const ve::MeshData mesh = ve::ChunkMeshFactory::create(chunk);



        require(chunk.solidBlockCount() > 0, "Demo terrain must contain solid blocks");

        require(mesh.vertexCount() > 0, "Demo terrain must produce vertices");

        require(mesh.indexCount() > 0, "Demo terrain must produce indices");

        require(mesh.indexCount() % 3 == 0, "Indices must form triangles");

        require(mesh.vertexCount() % 4 == 0, "Each exposed face must contain four vertices");



        const std::size_t exposedFaces = mesh.vertexCount() / 4;

        require(
            mesh.indexCount() == exposedFaces * 6,
            "Every exposed face must contain six indices"
        );


        std::cout << "[PASS] Demo terrain MeshData generation\n";

        std::cout << "[INFO] Solid Blocks: " << chunk.solidBlockCount() << '\n';
        std::cout << "[INFO] Exposed Faces: " << exposedFaces << '\n';
        std::cout << "[INFO] Vertices: " << mesh.vertexCount() << '\n';
        std::cout << "[INFO] Triangles: " << mesh.triangleCount() << '\n';
    }
}



int main() {
    try {
        testTerrainHeights();
        testChunkMeshGeneration();


        std::cout << "\nAll Chunk demo tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}