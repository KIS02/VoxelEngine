#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "assets/AssetManager.h"
#include "render/frame/FrameBuilder.h"
#include "scene/demo/DemoScene.h"
#include "scene/object/SceneObject.h"
#include "ui/UIManager.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    std::size_t expectedDrawCount( const ve::DemoScene& scene ) {
        std::size_t count = 1;
        scene.chunkMeshes().forEachMaterialMesh(
            [&]( const ve::ChunkPosition&, ve::BlockMaterialKey, const ve::MeshData&,
                 std::uint64_t, std::uint64_t ) {
                ++count;
            }
        );
        return count;
    }

    void requireCubeDraw( const ve::DemoScene& scene, const ve::AssetManager& assets,
                          const ve::RenderFrame& frame ) {
        std::size_t cubeObjectCount = 0;
        const ve::MaterialData* expectedMaterial = nullptr;
        scene.objects().forEach([&]( const ve::SceneObject& object ) {
            if (object.meshAssetId != scene.cubeMeshId())
                return;

            require(object.materialAssetId != 0, "Cube SceneObject must have a Material");
            expectedMaterial = &assets.material(object.materialAssetId);
            ++cubeObjectCount;
        });
        require(cubeObjectCount == 1, "Demo Scene must own one Cube with a stable Mesh ID");

        std::size_t cubeDrawCount = 0;
        for (const ve::RenderDraw& draw : frame.draws) {
            if (draw.meshId != scene.cubeMeshId())
                continue;

            require(draw.mesh == &scene.cubeMesh(), "Cube Draw must use the SceneObject Mesh");
            require(draw.material == expectedMaterial, "Cube Draw must use the SceneObject Material");
            ++cubeDrawCount;
        }
        require(cubeDrawCount == 1, "Demo Cube must be submitted exactly once");
    }

    void testFixedScene() {
        ve::AssetManager assets{};
        ve::DemoScene scene(assets);
        ve::UIManager ui{};

        require(scene.world().chunkCount() == 4, "Fixed demo must have four Chunks");
        require(scene.world().dirtyChunkCount() == 0, "Fixed demo must start clean");

        const ve::RenderFrame frame = ve::FrameBuilder::build(
            scene, assets, ui, 800, 600, false, ve::RenderBackend::Software
        );
        require(frame.draws.size() == expectedDrawCount(scene),
                "Fixed demo must submit each Material Submesh and one Cube");
        requireCubeDraw(scene, assets, frame);

        std::cout << "[PASS] Fixed demo contract\n";
    }

    void testProceduralScene() {
        ve::AssetManager assets{};
        ve::DemoScene scene(assets, ve::DemoTerrainMode::Procedural, 1234);
        ve::UIManager ui{};

        require(scene.world().chunkCount() == 9, "Procedural demo must have nine Chunks");
        require(scene.world().dirtyChunkCount() == 0, "Procedural demo must start clean");

        const ve::RenderFrame software = ve::FrameBuilder::build(
            scene, assets, ui, 800, 600, true, ve::RenderBackend::Software
        );
        const ve::RenderFrame opengl = ve::FrameBuilder::build(
            scene, assets, ui, 800, 600, true, ve::RenderBackend::OpenGL
        );

        require(software.draws.size() == expectedDrawCount(scene),
                "Procedural demo must submit each Material Submesh and one Cube");
        require(opengl.draws.size() == software.draws.size(),
                "OpenGL and Software must submit the same number of Draws");
        requireCubeDraw(scene, assets, software);
        requireCubeDraw(scene, assets, opengl);

        std::cout << "[PASS] Procedural demo draw contract\n";
    }
}


int main() {
    try {
        testFixedScene();
        testProceduralScene();

        std::cout << "All Phase9 Scene mode unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
