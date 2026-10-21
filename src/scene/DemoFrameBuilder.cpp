#include "scene/DemoFrameBuilder.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

#include "debug/DebugOverlayTelemetry.h"
#include "scene/DemoScene.h"
#include "world/BlockTypeName.h"


namespace ve
{
    RenderFrame DemoFrameBuilder::build( const DemoScene& scene, int width, int height,
                                          bool cursorCaptured, bool debugOverlayEnabled,
                                          RenderBackend backend, const DebugOverlayTelemetry& telemetry ) {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("Invalid demo framebuffer dimensions");
        }


        constexpr float pi = 3.14159265358979323846f;
        constexpr float fovYRadians = 60.0f * pi / 180.0f;

        const float aspectRatio =
            static_cast<float>(width) / static_cast<float>(height);


        RenderFrame frame{};

        frame.clearColor = makeColor(20, 31, 77);

        frame.view = scene.camera().viewMatrix();
        frame.projection = Mat4::perspective(
            fovYRadians,
            aspectRatio,
            0.1f,
            100.0f
        );


        // 2D Crosshair

        frame.crosshair.enabled = cursorCaptured;
        frame.crosshair.color = makeColor(255, 255, 255);
        frame.crosshair.armLength = 7;
        frame.crosshair.thickness = 3;


        // Selected Block Outline

        if (cursorCaptured && scene.selectedBlock().has_value()) {
            const WorldBlockPosition& position = scene.selectedBlock()->block;

            frame.selectionOutline.enabled = true;

            frame.selectionOutline.blockMin = Vec3{
                static_cast<float>(position.x),
                static_cast<float>(position.y),
                static_cast<float>(position.z)
            };

            frame.selectionOutline.color = makeColor(255, 0, 255);
            frame.selectionOutline.expansion = 0.03f;
        }


        // World Chunk Meshes

        scene.chunkMeshes().forEachMesh([&scene, &frame]( const ChunkPosition& position, const MeshData& mesh,
                                                         std::uint64_t meshId, std::uint64_t revision ) {
            const Vec3 translation{
                static_cast<float>(position.x) * static_cast<float>(Chunk::Size),
                static_cast<float>(position.y) * static_cast<float>(Chunk::Size),
                static_cast<float>(position.z) * static_cast<float>(Chunk::Size)
            };
            frame.draws.push_back(RenderDraw{
                &mesh, Mat4::translation(translation), revision, meshId, &scene.defaultMaterial()
            });
        });


        // Reference Cube

        const Mat4 cubeModel =
            Mat4::translation(Vec3{ 0.0f, 7.0f, 0.0f }) *
            Mat4::rotationY(30.0f * pi / 180.0f);

        frame.draws.push_back(RenderDraw{
            &scene.cubeMesh(),
            cubeModel,
            1,
            scene.cubeMeshId(),
            &scene.defaultMaterial()
        });


        // Debug Overlay Data

        if (debugOverlayEnabled) {
            DebugOverlayScene debugScene{};

            debugScene.backend = backend == RenderBackend::OpenGL
                ? BenchmarkBackend::OpenGL : BenchmarkBackend::Software;


            // Camera World Coordinates

            const Vec3 position = scene.camera().position();

            debugScene.cameraPosition = {
                position.x, position.y, position.z
            };


            // Camera Chunk Coordinates

            const float chunkSize = static_cast<float>(Chunk::Size);

            debugScene.chunkPosition = {
                static_cast<int>(std::floor(position.x / chunkSize)),
                static_cast<int>(std::floor(position.y / chunkSize)),
                static_cast<int>(std::floor(position.z / chunkSize))
            };


            // Selected Block Information

            if (scene.selectedBlock().has_value() && cursorCaptured) {
                const WorldBlockPosition& block = scene.selectedBlock()->block;

                std::ostringstream target;

                target << blockTypeName(scene.selectedBlock()->type)
                    << " (" << block.x
                    << ", " << block.y
                    << ", " << block.z << ")";

                debugScene.targetBlock = target.str();
            }


            // Pass Telemetry to the Shared RenderFrame

            frame.debugOverlay.enabled = true;
            frame.debugOverlay.lines = telemetry.makeLines(debugScene);
        }


        return frame;
    }


}
