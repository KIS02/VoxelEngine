#include <cassert>
#include <cmath>
#include <stdexcept>

#include "assets/AssetManager.h"
#include "assets/block/BlockRegistry.h"
#include "assets/geometry/primitives/CubeMeshGenerator.h"
#include "assets/geometry/primitives/CylinderMeshGenerator.h"
#include "assets/texture/UVTestTexture.h"


int main() {
    using namespace ve;
    static_assert(BlockRegistry::definition(BlockId::Grass).solid);
    static_assert(!BlockRegistry::definition(BlockId::Air).solid);
    assert(BlockRegistry::definition(BlockId::Stone).tint == makeColor(150, 150, 150));

    AssetManager assets{};
    const std::uint64_t cubeId = assets.addMesh(CubeMeshGenerator::create(1.0f));
    const std::uint64_t cylinderId = assets.addMesh(CylinderMeshGenerator::create(1.0f, 2.0f, 16));
    assert(cubeId != cylinderId);
    assert(assets.mesh(cubeId).triangleCount() == 12);
    assert(assets.mesh(cylinderId).triangleCount() == 64);

    const std::uint64_t texId = assets.addTexture(createUVTestTexture());
    const std::uint64_t materialId = assets.addMaterial(texId);
    assert(assets.material(materialId).texture == &assets.texture(texId));
    const MaterialData* ptr = &assets.material(materialId);
    for (int i = 0; i < 100; i++)
        assets.addMesh(CubeMeshGenerator::create(0.5f));
    assert(ptr == &assets.material(materialId));

    bool rejected = false;
    try {
        CylinderMeshGenerator::create(1.0f, 1.0f, 2);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);
    return 0;
}
