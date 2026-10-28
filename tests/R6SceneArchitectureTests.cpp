#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <type_traits>

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
}


int main() {
    static_assert(!std::is_copy_constructible_v<ve::DemoScene>);
    static_assert(!std::is_move_constructible_v<ve::DemoScene>);

    try {
        ve::AssetManager assets{};
        ve::DemoScene scene(assets);
        ve::UIManager ui{};

        require(scene.world().chunkCount() == 4, "Fixed scene must have four Chunks");
        require(scene.world().dirtyChunkCount() == 0, "Fixed scene must start clean");
        require(scene.rebuildDirtyMeshes() == 0, "Clean scene must not rebuild Meshes");
        require(scene.defaultMaterial().texture == &scene.texture(), "Default Material must reference its Texture");
        require(scene.cubeMeshId() != 0, "Cube Mesh ID must be valid");

        std::size_t cubeObjectCount = 0;
        const ve::MaterialData* cubeMaterial = nullptr;
        scene.objects().forEach([&]( const ve::SceneObject& object ) {
            if (object.meshAssetId != scene.cubeMeshId())
                return;

            require(object.materialAssetId != 0, "Cube SceneObject must have a Material");
            cubeMaterial = &assets.material(object.materialAssetId);
            ++cubeObjectCount;
        });
        require(cubeObjectCount == 1, "Scene must own exactly one Cube with its exposed Mesh ID");

        const ve::RenderFrame frame = ve::FrameBuilder::build(
            scene, assets, ui, 800, 600, true, ve::RenderBackend::Software
        );

        std::size_t expectedDraws = 1;
        scene.chunkMeshes().forEachMaterialMesh(
            [&]( const ve::ChunkPosition&, ve::BlockMaterialKey key, const ve::MeshData& mesh,
                 std::uint64_t meshId, std::uint64_t revision ) {
                ++expectedDraws;
                std::size_t matches = 0;

                for (const ve::RenderDraw& draw : frame.draws) {
                    if (draw.meshId != meshId)
                        continue;

                    require(draw.mesh == &mesh, "Voxel Draw must use its original Submesh");
                    require(draw.meshRevision == revision, "Voxel Draw revision must match its Submesh");
                    require(draw.material == &assets.material(scene.voxelMaterialId(key)),
                            "Voxel Draw must use its Material-specific asset");
                    ++matches;
                }

                require(matches == 1, "Each Voxel Material Submesh must be drawn exactly once");
            }
        );
        require(frame.draws.size() == expectedDraws,
                "Frame must contain all Voxel Submeshes and exactly one Cube Draw");

        const ve::RenderDraw* cubeDraw = nullptr;
        for (const ve::RenderDraw& draw : frame.draws) {
            if (draw.meshId != scene.cubeMeshId())
                continue;

            require(cubeDraw == nullptr, "Cube must not be drawn more than once");
            cubeDraw = &draw;
        }

        require(cubeDraw != nullptr, "Frame must contain the registered Cube SceneObject");
        require(cubeDraw->mesh == &scene.cubeMesh(), "Cube Draw must reference its registered Mesh");
        require(cubeDraw->material == cubeMaterial, "Cube Draw must reference its SceneObject Material");
        require(frame.ui.crosshair.enabled, "Captured cursor must show the Crosshair");
        require(!frame.ui.debugOverlay.enabled, "Debug Overlay must start disabled");

        const ve::RenderFrame benchmarkFrame = ve::FrameBuilder::build(
            scene, assets, ui, 800, 600, false, ve::RenderBackend::OpenGL
        );
        require(!benchmarkFrame.ui.crosshair.enabled && !benchmarkFrame.selectionOutline.enabled,
                "Uncaptured cursor must hide the Crosshair and Selection Outline");
        require(benchmarkFrame.draws.size() == frame.draws.size(),
                "OpenGL and Software must submit the same number of Draws");

        ui.setDebugEnabled(true);
        const ve::RenderFrame hudFrame = ve::FrameBuilder::build(
            scene, assets, ui, 800, 600, false, ve::RenderBackend::Software
        );
        require(hudFrame.ui.debugOverlay.enabled, "Debug Overlay must be enabled");
        require(!hudFrame.ui.debugOverlay.lines.empty(), "Debug Overlay must contain text");

        bool rejectedInvalidSize = false;
        try {
            ve::FrameBuilder::build(scene, assets, ui, 0, 600, false, ve::RenderBackend::Software);
        } catch (const std::invalid_argument&) {
            rejectedInvalidSize = true;
        }
        require(rejectedInvalidSize, "FrameBuilder must reject an invalid framebuffer size");

        const ve::Vec3 originalPosition = scene.camera().position();
        ve::InputActions movement{};
        movement.movement.forward = true;

        scene.update(0.5f, false, ve::MouseDelta{}, movement);
        require(scene.camera().position().x == originalPosition.x &&
                scene.camera().position().y == originalPosition.y &&
                scene.camera().position().z == originalPosition.z,
                "Uncaptured input must not move the Camera");

        scene.update(0.5f, true, ve::MouseDelta{}, movement);
        require(scene.camera().position().x != originalPosition.x ||
                scene.camera().position().z != originalPosition.z,
                "Captured movement must move the Camera");
        require(scene.world().dirtyChunkCount() == 0, "Camera movement must not dirty Chunks");

        std::cout << "[PASS] R6 Scene architecture contracts\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
