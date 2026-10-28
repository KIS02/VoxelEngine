#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "assets/AssetManager.h"
#include "render/frame/FrameBuilder.h"
#include "scene/SceneManager.h"
#include "scene/demo/DemoScene.h"
#include "scene/object/SceneObject.h"
#include "scene/voxel/VoxelWorld.h"
#include "ui/UIManager.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    std::size_t expectedDrawCount( const ve::Scene& scene ) {
        const ve::VoxelWorld* voxel = scene.voxelWorld();
        require(voxel != nullptr, "Demo Scene must own a VoxelWorld");

        std::size_t count = 1;
        voxel->meshes().forEachMaterialMesh(
            [&]( const ve::ChunkPosition&, ve::BlockMaterialKey, const ve::MeshData&,
                 std::uint64_t, std::uint64_t ) {
                ++count;
            }
        );
        return count;
    }

    void requireCubeDraw( const ve::Scene& scene, const ve::AssetManager& assets,
                          const ve::RenderFrame& frame, std::uint64_t cubeMeshId ) {
        std::size_t cubeObjectCount = 0;
        const ve::MaterialData* cubeMaterial = nullptr;
        scene.objects().forEach([&]( const ve::SceneObject& object ) {
            if (object.meshAssetId != cubeMeshId)
                return;

            require(object.materialAssetId != 0, "Cube SceneObject must have a Material");
            cubeMaterial = &assets.material(object.materialAssetId);
            ++cubeObjectCount;
        });
        require(cubeObjectCount == 1, "Active Scene must own exactly one registered Cube");

        std::size_t cubeDrawCount = 0;
        for (const ve::RenderDraw& draw : frame.draws) {
            if (draw.meshId != cubeMeshId)
                continue;

            require(draw.mesh == &assets.mesh(cubeMeshId), "Cube Draw must reference its registered Mesh");
            require(draw.material == cubeMaterial, "Cube Draw must reference its SceneObject Material");
            require(draw.material != nullptr && draw.material->texture != nullptr,
                    "Cube Material must reference a Texture");
            ++cubeDrawCount;
        }
        require(cubeDrawCount == 1, "Active Scene Cube must be submitted exactly once");
    }

    void requireNoMeshDraw( const ve::RenderFrame& frame, std::uint64_t excludedMeshId ) {
        for (const ve::RenderDraw& draw : frame.draws)
            require(draw.meshId != excludedMeshId, "Inactive Scene Mesh leaked into RenderFrame");
    }
}


int main() {
    try {
        ve::AssetManager assets{};
        ve::SceneManager scenes{};
        auto first = std::make_unique<ve::DemoScene>(assets);
        auto second = std::make_unique<ve::DemoScene>(assets);

        require(first->cubeMeshId() != second->cubeMeshId(), "Scenes must have distinct Cube Mesh IDs");
        const std::uint64_t firstId = first->cubeMeshId();
        const std::uint64_t secondId = second->cubeMeshId();

        require(first->voxelWorld() != second->voxelWorld(), "Scenes must own independent VoxelWorlds");
        require(first->voxelWorld()->blocks().chunkCount() == 4, "First Scene must have four Chunks");
        require(second->voxelWorld()->blocks().chunkCount() == 4, "Second Scene must have four Chunks");
        require(scenes.add("first", std::move(first)), "First Scene registration failed");
        require(scenes.add("second", std::move(second)), "Second Scene registration failed");
        require(scenes.requestActivation("first"), "First Scene activation request failed");
        require(scenes.applyPendingActivation(), "First Scene activation failed");

        ve::UIManager ui{};
        const ve::RenderFrame firstFrame = ve::FrameBuilder::build(
            *scenes.active(), assets, ui, 800, 600, true, ve::RenderBackend::Software
        );
        const std::size_t firstExpected = expectedDrawCount(*scenes.active());
        require(firstFrame.draws.size() == firstExpected, "First Scene Draw count changed");
        require(firstExpected >= 5, "First Scene must contain all Voxel and Object Draws");
        requireCubeDraw(*scenes.active(), assets, firstFrame, firstId);
        requireNoMeshDraw(firstFrame, secondId);

        require(scenes.requestActivation("second"), "Second Scene activation request failed");
        require(scenes.activeName() == "first", "Scene activation must be deferred");
        require(scenes.applyPendingActivation(), "Second Scene activation failed");
        require(scenes.remove("first"), "Inactive First Scene removal failed");

        const ve::RenderFrame secondFrame = ve::FrameBuilder::build(
            *scenes.active(), assets, ui, 800, 600, true, ve::RenderBackend::OpenGL
        );
        require(secondFrame.draws.size() == expectedDrawCount(*scenes.active()),
                "Second Scene Draw count changed");
        require(secondFrame.draws.size() == firstFrame.draws.size(),
                "Scene switch must preserve the Demo Draw count");
        requireCubeDraw(*scenes.active(), assets, secondFrame, secondId);
        requireNoMeshDraw(secondFrame, firstId);

        scenes.active()->camera().setProjectionMode(ve::Camera::ProjectionMode::Orthographic);
        const ve::RenderFrame ortho = ve::FrameBuilder::build(
            *scenes.active(), assets, ui, 800, 600, false, ve::RenderBackend::Software
        );
        require(ortho.projection(3, 3) == 1.0f, "Orthographic projection must preserve homogeneous W");
        require(!ortho.ui.crosshair.enabled, "Uncaptured cursor must hide the Crosshair");
        require(ortho.draws.size() == secondFrame.draws.size(),
                "Projection change must preserve the Draw count");
        requireCubeDraw(*scenes.active(), assets, ortho, secondId);

        ve::Entity chest{};
        chest.kind = ve::EntityKind::InteractiveObject;
        chest.gridPlacement = ve::GridPlacement{ ve::WorldBlockPosition{ 3, 1, 2 } };
        const ve::EntityId entityId = scenes.active()->entities().create(chest);
        const ve::Entity* created = scenes.active()->entities().find(entityId);
        require(created != nullptr && created->gridPlacement.has_value(),
                "Interactive Entity must preserve its GridPlacement");
        require(created->gridPlacement->position.x == 3, "Interactive Entity X coordinate changed");

        std::cout << "[PASS] V5 Scene integration contracts\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
