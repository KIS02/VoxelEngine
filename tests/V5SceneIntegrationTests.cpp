#include <cassert>
#include <memory>

#include "assets/AssetManager.h"
#include "render/frame/FrameBuilder.h"
#include "scene/SceneManager.h"
#include "scene/demo/DemoScene.h"
#include "scene/voxel/VoxelWorld.h"
#include "ui/UIManager.h"


int main() {
    ve::AssetManager assets{};
    ve::SceneManager scenes{};
    auto first = std::make_unique<ve::DemoScene>(assets);
    auto second = std::make_unique<ve::DemoScene>(assets);
    assert(first->cubeMeshId() != second->cubeMeshId());
    const std::uint64_t firstId = first->cubeMeshId();
    const std::uint64_t secondId = second->cubeMeshId();
    assert(first->voxelWorld() != second->voxelWorld());
    assert(first->voxelWorld()->blocks().chunkCount() == 4);
    assert(second->voxelWorld()->blocks().chunkCount() == 4);
    assert(scenes.add("first", std::move(first)));
    assert(scenes.add("second", std::move(second)));
    assert(scenes.requestActivation("first"));
    assert(scenes.applyPendingActivation());

    ve::UIManager ui{};
    ve::RenderFrame firstFrame = ve::FrameBuilder::build(
        *scenes.active(), assets, ui, 800, 600, true, ve::RenderBackend::Software
    );
    assert(firstFrame.draws.size() == 5);
    assert(firstFrame.draws.back().meshId == firstId);
    assert(firstFrame.draws.back().material != nullptr);
    assert(firstFrame.draws.back().material->texture != nullptr);

    assert(scenes.requestActivation("second"));
    assert(scenes.activeName() == "first");
    assert(scenes.applyPendingActivation());
    assert(scenes.remove("first"));
    ve::RenderFrame secondFrame = ve::FrameBuilder::build(
        *scenes.active(), assets, ui, 800, 600, true, ve::RenderBackend::OpenGL
    );
    assert(secondFrame.draws.size() == 5);
    assert(secondFrame.draws.back().meshId == secondId);

    scenes.active()->camera().setProjectionMode(ve::Camera::ProjectionMode::Orthographic);
    const ve::RenderFrame ortho = ve::FrameBuilder::build(
        *scenes.active(), assets, ui, 800, 600, false, ve::RenderBackend::Software
    );
    assert(ortho.projection(3, 3) == 1.0f);
    assert(!ortho.ui.crosshair.enabled);
    assert(ortho.draws.size() == 5);

    ve::Entity chest{};
    chest.kind = ve::EntityKind::InteractiveObject;
    chest.gridPlacement = ve::GridPlacement{ ve::WorldBlockPosition{ 3, 1, 2 } };
    const ve::EntityId entityId = scenes.active()->entities().create(chest);
    assert(scenes.active()->entities().find(entityId)->gridPlacement->position.x == 3);
    return 0;
}
