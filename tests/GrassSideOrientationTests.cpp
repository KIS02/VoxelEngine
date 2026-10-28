#include <cmath>
#include <iostream>
#include <stdexcept>

#include "assets/block/BlockDefinition.h"
#include "voxel/chunk/Chunk.h"
#include "voxel/chunk/ChunkMeshBuilder.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        ve::Chunk chunk;
        chunk.setBlock(0, 0, 0, ve::BlockId::Grass);

        const ve::MaterialMeshes groups = ve::ChunkMeshBuilder::createMaterialMeshes(chunk);
        const ve::MeshData& side = groups.at(ve::BlockMaterialKey::GrassSide);

        require(side.vertices().size() == 16, "Grass must have four side quads");

        for (std::size_t index = 0; index < side.vertices().size(); index += 4) {
            const auto& a = side.vertices()[index];
            const auto& b = side.vertices()[index + 1];
            const auto& c = side.vertices()[index + 2];
            const auto& d = side.vertices()[index + 3];

            require(std::fabs(a.position.y - b.position.y) < 0.0001f,
                    "U-axis must be horizontal");

            require(std::fabs(c.position.y - d.position.y) < 0.0001f,
                    "Upper edge must be horizontal");

            require(c.position.y > b.position.y, "V-axis must point upward");

            require(a.uv.y == 0.0f && b.uv.y == 0.0f,
                    "Bottom edge UV mismatch");

            require(c.uv.y == 1.0f && d.uv.y == 1.0f,
                    "Top edge UV mismatch");
        }

        std::cout << "[PASS] Grass Side UV orientation on all four horizontal faces\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}