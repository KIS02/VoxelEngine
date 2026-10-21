#include <stdexcept>
#include <type_traits>

#include "assets/AssetManager.h"
#include "ui/UIManager.h"
#include "render/frame/FrameBuilder.h"
#include "scene/demo/DemoScene.h"


namespace
{
    void require( bool condition ) {
        if (!condition)
            throw std::runtime_error("R6 scene architecture contract failed");
    }
}


int main() {
    static_assert(!std::is_copy_constructible_v<ve::DemoScene>);
    static_assert(!std::is_move_constructible_v<ve::DemoScene>);

    ve::AssetManager assets{};
    ve::DemoScene scene(assets);
    ve::UIManager ui{};
    require(scene.world().chunkCount() == 4);
    require(scene.world().dirtyChunkCount() == 0);
    require(scene.rebuildDirtyMeshes() == 0);
    require(scene.defaultMaterial().texture == &scene.texture());
    require(scene.cubeMeshId() != 0);

    const ve::RenderFrame frame = ve::FrameBuilder::build(scene, assets, ui, 800, 600, true, ve::RenderBackend::Software);
    require(frame.draws.size() == 5);
    require(frame.ui.crosshair.enabled);
    require(!frame.ui.debugOverlay.enabled);
    require(frame.draws.back().mesh == &scene.cubeMesh());
    require(frame.draws.back().material == &scene.defaultMaterial());
    require(frame.draws.back().meshId == scene.cubeMeshId());

    const ve::RenderFrame benchmarkFrame = ve::FrameBuilder::build(scene, assets, ui, 800, 600, false, ve::RenderBackend::OpenGL);
    require(!benchmarkFrame.ui.crosshair.enabled && !benchmarkFrame.selectionOutline.enabled);
    require(benchmarkFrame.draws.size() == frame.draws.size());

    ui.setDebugEnabled(true);
    const ve::RenderFrame hudFrame = ve::FrameBuilder::build(
        scene, assets, ui, 800, 600, false, ve::RenderBackend::Software
    );
    require(hudFrame.ui.debugOverlay.enabled);
    require(!hudFrame.ui.debugOverlay.lines.empty());

    bool rejectedInvalidSize = false;
    try {
        ve::FrameBuilder::build(scene, assets, ui, 0, 600, false, ve::RenderBackend::Software);
    } catch (const std::invalid_argument&) {
        rejectedInvalidSize = true;
    }
    require(rejectedInvalidSize);

    const ve::Vec3 originalPosition = scene.camera().position();
    ve::InputActions movement{};
    movement.movement.forward = true;
    scene.update(0.5f, false, ve::MouseDelta{}, movement);
    require(scene.camera().position().x == originalPosition.x);
    require(scene.camera().position().y == originalPosition.y);
    require(scene.camera().position().z == originalPosition.z);

    scene.update(0.5f, true, ve::MouseDelta{}, movement);
    require(scene.camera().position().x != originalPosition.x ||
            scene.camera().position().z != originalPosition.z);
    require(scene.world().dirtyChunkCount() == 0);
    return 0;
}
