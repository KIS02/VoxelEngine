#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include "assets/AssetManager.h"
#include "input/ApplicationInput.h"
#include "render/frame/FrameBuilder.h"
#include "scene/camera/Camera.h"
#include "scene/demo/DemoScene.h"
#include "ui/UIManager.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    ve::ShadingModel otherModel( ve::ShadingModel model ) {
        return model == ve::ShadingModel::BlinnPhong
            ? ve::ShadingModel::Pbr : ve::ShadingModel::BlinnPhong;
    }

    void requireSameDraws( const ve::RenderFrame& before, const ve::RenderFrame& after ) {
        require(before.draws.size() == after.draws.size(),
                "Changing shading mode must not change draw count");

        for (std::size_t i = 0; i < before.draws.size(); ++i) {
            const ve::RenderDraw& first = before.draws[i];
            const ve::RenderDraw& second = after.draws[i];
            require(first.mesh == second.mesh && first.meshId == second.meshId &&
                    first.meshRevision == second.meshRevision && first.material == second.material,
                    "Changing shading mode must not rebuild or replace render assets");
        }
    }

    void requireOverlay( const ve::RenderFrame& frame ) {
        require(frame.ui.debugOverlay.enabled, "F3 overlay was not enabled");
        const std::string expected = frame.shadingModel == ve::ShadingModel::Pbr
            ? "SHADING: PBR" : "SHADING: BLINN-PHONG";
        bool found = false;
        for (const std::string& line : frame.ui.debugOverlay.lines) {
            if (line == expected)
                found = true;
        }
        require(found, "HUD does not report the Frame's shading model");
    }

    void testFixedSceneToggle() {
        ve::AssetManager assets{};
        ve::DemoScene scene(assets);
        ve::UIManager ui{};
        ui.setDebugEnabled(true);

        const ve::ShadingModel initial = scene.shadingModel();
        const ve::Camera::ProjectionMode projection = scene.camera().projectionMode();
        const ve::Vec3 cameraPosition = scene.camera().position();
        const ve::RenderFrame before = ve::FrameBuilder::build(
            scene, assets, ui, 800, 600, false, ve::RenderBackend::Software
        );
        require(before.shadingModel == initial, "FrameBuilder ignored the Scene shading state");
        requireOverlay(before);

        ve::InputActions actions{};
        actions.toggleShadingModel = true;
        scene.update(0.0f, false, ve::MouseDelta{}, actions);
        require(scene.shadingModel() == otherModel(initial), "M action did not change the Scene state");

        const ve::RenderFrame software = ve::FrameBuilder::build(
            scene, assets, ui, 800, 600, false, ve::RenderBackend::Software
        );
        const ve::RenderFrame opengl = ve::FrameBuilder::build(
            scene, assets, ui, 800, 600, false, ve::RenderBackend::OpenGL
        );
        require(software.shadingModel == otherModel(initial) &&
                opengl.shadingModel == software.shadingModel,
                "Software and OpenGL must receive the same new ShadingModel");
        requireSameDraws(before, software);
        requireSameDraws(before, opengl);
        requireOverlay(software);
        requireOverlay(opengl);
        require(scene.camera().projectionMode() == projection,
                "Shading toggle changed the projection mode");
        require(scene.camera().position().x == cameraPosition.x &&
                scene.camera().position().y == cameraPosition.y &&
                scene.camera().position().z == cameraPosition.z,
                "Shading toggle changed the camera position");

        actions.toggleShadingModel = false;
        scene.update(0.0f, false, ve::MouseDelta{}, actions);
        require(scene.shadingModel() == otherModel(initial),
                "Shading state must persist between input events");

        actions.toggleShadingModel = true;
        scene.update(0.0f, false, ve::MouseDelta{}, actions);
        require(scene.shadingModel() == initial, "Second M action must restore initial mode");

        const ve::RenderFrame restored = ve::FrameBuilder::build(
            scene, assets, ui, 800, 600, false, ve::RenderBackend::Software
        );
        require(restored.shadingModel == initial, "Restored mode was not sent to RenderFrame");
        requireSameDraws(before, restored);
        requireOverlay(restored);
        std::cout << "[PASS] Scene state, RenderFrame, CPU/GPU parity, Mesh Cache and F3 HUD\n";
    }
}


int main() {
    try {
        testFixedSceneToggle();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
