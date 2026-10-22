#include <iostream>
#include <stdexcept>

#include "assets/AssetManager.h"
#include "render/frame/FrameBuilder.h"
#include "scene/demo/DemoScene.h"
#include "ui/UIManager.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
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

        require(frame.draws.size() == 5, "Fixed demo draw contract changed");

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

        require(software.draws.size() == 10,
                "Procedural scene must have 9 Chunk draws and 1 object draw");

        require(opengl.draws.size() == software.draws.size(),
                "Backend draw paths must agree");

        require(software.draws.back().meshId == scene.cubeMeshId(),
                "Demo scene object draw must remain");

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