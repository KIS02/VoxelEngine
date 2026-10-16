#include "Application.h"

#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>
#include <sstream>

#include "demo/ChunkDemoFactory.h"

#include "math/Mat4.h"
#include "math/Vec3.h"

#include "render/Color.h"

#include "geometry/CubeMeshFactory.h"
#include "render/texture/UVTestTexture.h"

#include "render/backends/opengl/OpenGLRenderBackend.h"
#include "render/backends/software/SoftwareRenderBackend.h"

#include "world/BlockInteractor.h"

#include "benchmark/BenchmarkCpuTimer.h"
#include "benchmark/BenchmarkRenderStatisticsCollector.h"
#include "benchmark/BenchmarkRunController.h"
#include "benchmark/BenchmarkFrameCollector.h"
#include "benchmark/BenchmarkSummaryPrinter.h"
#include "benchmark/BenchmarkCsvWriter.h"

namespace ve
{
    namespace
    {
        const char* blockTypeName( BlockType type ) {
            switch (type) {
                case BlockType::Air: return "Air";
                case BlockType::Grass: return "Grass";
                case BlockType::Dirt: return "Dirt";
                case BlockType::Stone: return "Stone";
            }

            return "Unknown";
        }



        bool sameBlockPosition( const WorldBlockPosition& a, const WorldBlockPosition& b ) {
            return a.x == b.x && a.y == b.y && a.z == b.z;
        }
    }



    Application::Application( RenderBackend backend )
        : backend_(backend),
          window_(800, 600, "Voxel Engine", backend == RenderBackend::OpenGL
              ? WindowClientApi::OpenGL : WindowClientApi::None),
          frameBuffer_(800, 600), depthBuffer_(800, 600),
          rasterizer_(frameBuffer_, depthBuffer_), viewport_(800, 600),
          presenter_(window_.nativeHandle()),
          cubeMesh_(CubeMeshFactory::create(1.0f)),
          chunkMeshManager_(meshIdGenerator_),
          uvTexture_(createUVTestTexture()) {



        defaultMaterial_.texture = &uvTexture_;

        // Camera

        camera_.setPosition(Vec3{ 8.5f, 6.5f, 10.5f });
        camera_.lookAt(Vec3{ 8.5f, 2.5f, 8.5f });

        cameraController_.setMoveSpeed(8.0f);
        cameraController_.setMouseSensitivity(0.1f);



        // World

        populateDemoWorld();
        chunkMeshManager_.rebuildDirtyMeshes(world_);
        cubeMeshId_ = meshIdGenerator_.generate();



        // Rendering Backend

        if (backend_ == RenderBackend::OpenGL) {
            renderBackendDriver_ = std::make_unique<OpenGLRenderBackend>(
                window_, openGLRenderer_, uvTexture_
            );
        }
        else if (backend_ == RenderBackend::Software) {
            renderBackendDriver_ = std::make_unique<SoftwareRenderBackend>(
                frameBuffer_, depthBuffer_, rasterizer_, viewport_, presenter_, uvTexture_
            );
        }
        else {
            throw std::invalid_argument("Unsupported Render Backend");
        }



        renderBackendDriver_->initialize();

        window_.setCursorCaptured(true);



        // Diagnostics

        std::cout << "[Application] Render Backend initialized" << std::endl;

        std::cout << "[World] " << world_.chunkCount()
                  << " Chunks created" << std::endl;

        std::cout << "[Camera] Position ("
                  << camera_.position().x << ", "
                  << camera_.position().y << ", "
                  << camera_.position().z << ")"
                  << std::endl;

        std::cout << "[Camera] Movement Speed: "
                  << cameraController_.moveSpeed()
                  << " units/second" << std::endl;

        std::cout << "[Camera] Mouse Sensitivity: "
                  << cameraController_.mouseSensitivity()
                  << " degrees/pixel" << std::endl;

        std::cout << "[Raycast] Maximum Reach: "
                  << blockReach_ << " blocks" << std::endl;

        std::cout << "[Controls] WASD: Move | Space: Up | Left Shift: Down"
                  << std::endl;

        std::cout << "[Controls] Mouse: Look | Left Click: Break Block"
                  << " | Right Click: Place Dirt"
                  << " | Escape: Release/Exit"
                  << std::endl;

        std::cout << "[Controls] Left Click with Free Cursor: Capture Mouse"
                  << std::endl;

        std::cout << "[Overlay] Crosshair and Selection Outline enabled in FPS mode"
                  << std::endl;

        if (!chunkRemovalDemoEnabled_) {
            std::cout << "[Demo] Automatic Chunk removal disabled"
                      << std::endl;
        }
    }



    void Application::populateDemoWorld() {
        for (int z = -1; z <= 0; ++z) {
            for (int x = -1; x <= 0; ++x) {
                const ChunkPosition position{ x, 0, z };

                Chunk& chunk = world_.getOrCreateChunk(position);

                chunk = createDemoChunk();

                world_.markChunkDirty(position);
            }
        }
    }



    void Application::updateBlockSelection() {
        const std::optional<VoxelRaycastHit> hit = VoxelRaycaster::cast(
            world_,
            camera_.position(),
            camera_.forward(),
            blockReach_
        );



        if (!hit.has_value()) {
            if (selectedBlock_.has_value()) {
                std::cout << "[Raycast] No block in range" << std::endl;
            }

            selectedBlock_.reset();

            return;
        }



        const bool selectionChanged =
            !selectedBlock_.has_value() ||
            !sameBlockPosition(selectedBlock_->block, hit->block) ||
            !sameBlockPosition(selectedBlock_->faceNormal, hit->faceNormal) ||
            selectedBlock_->type != hit->type;



        selectedBlock_ = hit;



        if (!selectionChanged) {
            return;
        }



        std::cout << "[Raycast] Hit " << blockTypeName(hit->type)
                  << " | Block: ("
                  << hit->block.x << ", "
                  << hit->block.y << ", "
                  << hit->block.z << ")"
                  << " | Normal: ("
                  << hit->faceNormal.x << ", "
                  << hit->faceNormal.y << ", "
                  << hit->faceNormal.z << ")"
                  << " | Distance: " << hit->distance
                  << std::endl;
    }



    RenderFrame Application::createDemoFrame( int width, int height ) const {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("Invalid demo framebuffer dimensions");
        }



        constexpr float pi = 3.14159265358979323846f;
        constexpr float fovYRadians = 60.0f * pi / 180.0f;

        const float aspectRatio =
            static_cast<float>(width) / static_cast<float>(height);



        RenderFrame frame{};

        frame.clearColor = makeColor(20, 31, 77);

        frame.view = camera_.viewMatrix();
        frame.projection = Mat4::perspective(
            fovYRadians,
            aspectRatio,
            0.1f,
            100.0f
        );



        // 2D Crosshair

        frame.crosshair.enabled = window_.isCursorCaptured();
        frame.crosshair.color = makeColor(255, 255, 255);
        frame.crosshair.armLength = 7;
        frame.crosshair.thickness = 3;



        // Selected Block Outline

        if (window_.isCursorCaptured() && selectedBlock_.has_value()) {
            const WorldBlockPosition& position = selectedBlock_->block;

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

        chunkMeshManager_.appendDraws(frame, defaultMaterial_);


        // Reference Cube

        const Mat4 cubeModel =
            Mat4::translation(Vec3{ 0.0f, 7.0f, 0.0f }) *
            Mat4::rotationY(30.0f * pi / 180.0f);

        frame.draws.push_back(RenderDraw{
            &cubeMesh_,
            cubeModel,
            1,
            cubeMeshId_,
            &defaultMaterial_
        });






        // Debug Overlay Data

        if (debugOverlayEnabled_) {
            DebugOverlayScene debugScene{};

            debugScene.backend = backend_ == RenderBackend::OpenGL
                ? BenchmarkBackend::OpenGL : BenchmarkBackend::Software;


            // Camera World Coordinates

            const Vec3 position = camera_.position();

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

            if (selectedBlock_.has_value() && window_.isCursorCaptured()) {
                const WorldBlockPosition& block = selectedBlock_->block;

                std::ostringstream target;

                target << blockTypeName(selectedBlock_->type)
                    << " (" << block.x
                    << ", " << block.y
                    << ", " << block.z << ")";

                debugScene.targetBlock = target.str();
            }


            // Pass Telemetry to the Shared RenderFrame

            frame.debugOverlay.enabled = true;
            frame.debugOverlay.lines = debugTelemetry_.makeLines(debugScene);
        }






        return frame;
    }


    void Application::processInput() {
        // Input Requests Are Rebuilt Every Frame

        breakBlockRequested_ = false;
        placeBlockRequested_ = false;


        const bool f3Pressed = window_.isKeyPressed(Key::F3);

        if (f3Pressed && !f3KeyWasPressed_) {
            debugOverlayEnabled_ = !debugOverlayEnabled_;

            std::cout << "[DebugOverlay] "
                    << (debugOverlayEnabled_ ? "Enabled" : "Disabled")
                    << '\n';
        }

        f3KeyWasPressed_ = f3Pressed;


        // Escape: Release Cursor or Exit

        const bool escapePressed = window_.isKeyPressed(Key::Escape);

        if (escapePressed && !escapeKeyWasPressed_) {
            if (window_.isCursorCaptured()) {
                window_.setCursorCaptured(false);

                std::cout << "[Input] Cursor released" << std::endl;
            }
            else {
                window_.requestClose();
            }
        }

        escapeKeyWasPressed_ = escapePressed;



        // Detect Left Mouse Button Press Edge

        const bool leftMousePressed =
            window_.isMouseButtonPressed(MouseButton::Left);

        const bool leftMouseJustPressed =
            leftMousePressed && !leftMouseButtonWasPressed_;



        if (!escapePressed && leftMouseJustPressed) {
            if (!window_.isCursorCaptured()) {
                // The First Click Only Recaptures the Mouse

                window_.setCursorCaptured(true);

                std::cout << "[Input] Cursor captured" << std::endl;
            }
            else {
                breakBlockRequested_ = true;
            }
        }



        leftMouseButtonWasPressed_ = leftMousePressed;



        // Detect Right Mouse Button Press Edge

        const bool rightMousePressed =
            window_.isMouseButtonPressed(MouseButton::Right);

        const bool rightMouseJustPressed =
            rightMousePressed && !rightMouseButtonWasPressed_;



        // Left Mouse Button Has Priority if Both Buttons Are Pressed

        if (!escapePressed &&
            !leftMouseJustPressed &&
            !breakBlockRequested_ &&
            rightMouseJustPressed &&
            window_.isCursorCaptured()) {

            placeBlockRequested_ = true;
        }



        rightMouseButtonWasPressed_ = rightMousePressed;
    }

    void Application::update( float deltaTime ) {
        // Camera Movement and Mouse Look

        if (window_.isCursorCaptured()) {
            const MouseDelta mouseDelta = window_.mouseDelta();

            cameraController_.updateLook(camera_, mouseDelta);



            CameraMovementInput input{};

            input.forward = window_.isKeyPressed(Key::W);
            input.backward = window_.isKeyPressed(Key::S);

            input.left = window_.isKeyPressed(Key::A);
            input.right = window_.isKeyPressed(Key::D);

            input.up = window_.isKeyPressed(Key::Space);
            input.down = window_.isKeyPressed(Key::LeftShift);



            cameraController_.update(camera_, input, deltaTime);
        }



        // Break or Place a Block Using the Current Camera Direction

        if (window_.isCursorCaptured()) {
            if (breakBlockRequested_) {
                const std::optional<VoxelRaycastHit> broken = BlockInteractor::breakBlock(
                    world_,
                    camera_.position(),
                    camera_.forward(),
                    blockReach_
                );



                if (broken.has_value()) {
                    std::cout << "[Block] Broken " << blockTypeName(broken->type)
                            << " | Position: ("
                            << broken->block.x << ", "
                            << broken->block.y << ", "
                            << broken->block.z << ")"
                            << " | Dirty Chunks: " << world_.dirtyChunkCount()
                            << std::endl;
                }
            }
            else if (placeBlockRequested_) {
                const std::optional<BlockPlacementResult> placed = BlockInteractor::placeBlock(
                    world_,
                    camera_.position(),
                    camera_.forward(),
                    blockReach_,
                    BlockType::Dirt
                );



                if (placed.has_value()) {
                    std::cout << "[Block] Placed " << blockTypeName(placed->type)
                            << " | Position: ("
                            << placed->placedBlock.x << ", "
                            << placed->placedBlock.y << ", "
                            << placed->placedBlock.z << ")"
                            << " | Attached To: ("
                            << placed->attachedTo.x << ", "
                            << placed->attachedTo.y << ", "
                            << placed->attachedTo.z << ")"
                            << " | Dirty Chunks: " << world_.dirtyChunkCount()
                            << std::endl;
                }
            }
        }



        // Consume Both Input Requests

        breakBlockRequested_ = false;
        placeBlockRequested_ = false;



        // Existing Optional Chunk Removal Demo

        if (chunkRemovalDemoEnabled_ && !pitOpen_) {
            editElapsed_ += deltaTime;



            if (editElapsed_ >= 3.0f) {
                const ChunkPosition targetPosition{ 0, 0, 0 };

                if (!world_.removeChunk(targetPosition)) {
                    throw std::runtime_error("Target Chunk does not exist");
                }



                chunkMeshManager_.remove(targetPosition);

                pitOpen_ = true;



                std::cout << "[World] Chunk (0, 0, 0) REMOVED"
                        << " | Remaining Chunks: " << world_.chunkCount()
                        << " | Dirty Chunks: " << world_.dirtyChunkCount()
                        << std::endl;
            }
        }



        // Refresh the Selected Block After All World Changes

        updateBlockSelection();
    }

    void Application::render( BenchmarkCpuTimings& cpuTimings, double& backendCallWallMs,
                            std::uint64_t frameId, BenchmarkRenderStatistics& renderStatistics ) {
        // Dirty Chunk Mesh Rebuild

        std::uint64_t rebuiltChunkMeshes = 0;

        {
            BenchmarkCpuScope scope(cpuTimings.meshRebuildMs);
            rebuiltChunkMeshes = chunkMeshManager_.rebuildDirtyMeshes(world_);
        }


        renderStatistics.residentChunkCount = world_.chunkCount();
        renderStatistics.meshRebuildCount = rebuiltChunkMeshes;


        // RenderFrame Assembly and Geometry Statistics

        std::optional<RenderFrame> frame;

        {
            BenchmarkCpuScope scope(cpuTimings.frameAssemblyMs);

            const RenderSize size = renderBackendDriver_->framebufferSize();

            if (size.width > 0 && size.height > 0) {
                frame.emplace(createDemoFrame(size.width, size.height));

                renderStatistics = BenchmarkRenderStatisticsCollector::collect(
                    *frame, static_cast<std::uint64_t>(world_.chunkCount())
                );

                renderStatistics.meshRebuildCount = rebuiltChunkMeshes;
            }
        }


        if (!frame.has_value())
            return;


        // Backend Rendering and Presentation

        {
            BenchmarkCpuScope scope(backendCallWallMs);

            renderBackendDriver_->renderMeasured(
                *frame, cpuTimings, frameId, &renderStatistics
            );
        }
    }

    void Application::run( bool benchmarkMode ) {
        std::cout << "Voxel Engine Started" << '\n';


        BenchmarkRunController benchmarkController{};
        BenchmarkFrameCollector benchmarkCollector{};

        std::uint64_t expectedChunkCount = 0;


        // Configure Fixed Benchmark Scene

        if (benchmarkMode) {
            constexpr int benchmarkWidth = 800;
            constexpr int benchmarkHeight = 600;

            constexpr std::uint32_t warmupFrames = 120;
            constexpr std::uint32_t measurementFrames = 600;


            // Restore the Original Demo Camera

            camera_.setPosition(Vec3{ 8.5f, 6.5f, 10.5f });
            camera_.lookAt(Vec3{ 8.5f, 2.5f, 8.5f });


            // Disable Interaction and Overlays

            window_.setCursorCaptured(false);

            selectedBlock_.reset();

            breakBlockRequested_ = false;
            placeBlockRequested_ = false;

            chunkRemovalDemoEnabled_ = false;


            // Validate the Render Target

            const RenderSize size = renderBackendDriver_->framebufferSize();

            if (size.width != benchmarkWidth || size.height != benchmarkHeight)
                throw std::runtime_error("Benchmark requires an 800x600 framebuffer");


            // Validate Initial World State

            if (world_.dirtyChunkCount() != 0)
                throw std::runtime_error("Benchmark requires a clean initial World");

            expectedChunkCount = static_cast<std::uint64_t>(world_.chunkCount());


            // Configure OpenGL Swap Interval for Benchmarking

            if (backend_ == RenderBackend::OpenGL)
                window_.setSwapInterval(0);


            // Create Reproducible Run Configuration

            BenchmarkRunConfiguration configuration{};

            configuration.backend = backend_ == RenderBackend::OpenGL
                ? BenchmarkBackend::OpenGL : BenchmarkBackend::Software;

            configuration.scenarioName = "fixed_demo_world_4_chunks";

    #ifdef NDEBUG
            configuration.buildConfiguration = "NDEBUG build";
    #else
            configuration.buildConfiguration = "Non-NDEBUG build";
    #endif

            configuration.framebufferWidth = benchmarkWidth;
            configuration.framebufferHeight = benchmarkHeight;

            configuration.vsyncEnabled =
                backend_ == RenderBackend::OpenGL &&
                window_.requestedSwapInterval() == 1;

            configuration.crosshairEnabled = false;
            configuration.selectionOutlineEnabled = false;

            const Vec3 position = camera_.position();
            const Vec3 forward = camera_.forward();

            configuration.cameraPosition = { position.x, position.y, position.z };
            configuration.cameraForward = { forward.x, forward.y, forward.z };

            configuration.warmupFrameCount = warmupFrames;
            configuration.measurementFrameCount = measurementFrames;


            // Initialize Run Controller and Sample Collector

            benchmarkController.begin(configuration);

            const std::uint64_t firstMeasurementFrameId =
                static_cast<std::uint64_t>(warmupFrames) + 1;

            benchmarkCollector.begin(configuration, firstMeasurementFrameId);


            // Benchmark Environment Diagnostics

            std::cout << "[Benchmark] Fixed Mode Started"
                    << " | Backend " << (backend_ == RenderBackend::OpenGL ? "OpenGL" : "Software")
                    << " | Resolution " << benchmarkWidth << "x" << benchmarkHeight
                    << " | Chunks " << expectedChunkCount
                    << '\n';

            std::cout << "[Benchmark] Camera ("
                    << position.x << ", " << position.y << ", " << position.z << ")"
                    << " | Forward ("
                    << forward.x << ", " << forward.y << ", " << forward.z << ")"
                    << '\n';

            std::cout << "[Benchmark][Environment]"
                    << " | Build " << configuration.buildConfiguration
                    << " | Scene " << configuration.scenarioName
                    << '\n';

            if (backend_ == RenderBackend::OpenGL) {
                std::cout << "[Benchmark][VSync]"
                        << " | Requested Swap Interval " << window_.requestedSwapInterval()
                        << " | Actual synchronization not verified"
                        << '\n';
            } else {
                std::cout << "[Benchmark][VSync]"
                        << " | NotApplicable (Software Backend)"
                        << '\n';
            }

            std::cout << "[Benchmark] Warmup " << warmupFrames
                    << " frames | Measurement " << measurementFrames
                    << " frames"
                    << '\n';
        }


        std::uint64_t frameIndex = 0;


        while (!window_.shouldClose()) {
            BenchmarkCpuTimings cpuTimings{};
            BenchmarkRenderStatistics renderStatistics{};

            double backendCallWallMs = 0.0;

            std::uint64_t completedGpuFrameId = 0;
            BenchmarkGpuTiming completedGpuTiming{};

            bool gpuResultAvailable = false;


            // Entire Main Loop Iteration

            {
                BenchmarkCpuScope frameScope(cpuTimings.frameWallMs);


                // OS Event Polling

                {
                    BenchmarkCpuScope eventScope(cpuTimings.eventPollMs);
                    window_.pollEvents();
                }


                if (benchmarkMode && window_.shouldClose())
                    break;


                // Delta Time

                const float deltaTime = timer_.tick();


                // Normal Gameplay Only

                if (!benchmarkMode) {
                    {
                        BenchmarkCpuScope inputScope(cpuTimings.inputMs);
                        processInput();
                    }

                    {
                        BenchmarkCpuScope updateScope(cpuTimings.updateMs);
                        update(deltaTime);
                    }
                }


                // Rebuild Meshes and Render

                ++frameIndex;

                render(cpuTimings, backendCallWallMs, frameIndex, renderStatistics);


                // Non-blocking GPU Timing Result Collection

                gpuResultAvailable = renderBackendDriver_->tryPopGpuTiming(
                    completedGpuFrameId, completedGpuTiming
                );
            }


            
            // Update Live Debug Telemetry After the Frame Timer Has Stopped

            if (!benchmarkMode) {
                debugTelemetry_.recordFrame(cpuTimings, renderStatistics);

                if (gpuResultAvailable)
                    debugTelemetry_.observeGpuTiming(completedGpuTiming);
            }


            // Fixed Benchmark Mode

            if (benchmarkMode) {
                debugOverlayEnabled_ = false;
                f3KeyWasPressed_ = false;

                const RenderSize currentSize = renderBackendDriver_->framebufferSize();

                if (currentSize.width != 800 || currentSize.height != 600)
                    throw std::runtime_error("Benchmark framebuffer resolution changed");

                if (renderStatistics.residentChunkCount != expectedChunkCount)
                    throw std::runtime_error("Benchmark World Chunk count changed");

                if (renderStatistics.meshRebuildCount != 0)
                    throw std::runtime_error("Benchmark scene triggered an unexpected Mesh rebuild");


                const bool measurementFrame = benchmarkController.shouldRecordCurrentFrame();


                // Resources Must Remain Stable During Measurement

                if (measurementFrame &&
                    (renderStatistics.gpuMeshUploadCount != 0 ||
                    renderStatistics.gpuMeshReleaseCount != 0)) {

                    throw std::runtime_error("Benchmark GPU resources changed during measurement");
                }


                // Store Only Measurement Frames

                if (measurementFrame) {
                    benchmarkCollector.recordFrame(
                        frameIndex, cpuTimings, renderStatistics
                    );
                }


                // Associate GPU Results With Their Original Frame IDs

                if (gpuResultAvailable) {
                    benchmarkCollector.attachGpuResult(
                        completedGpuFrameId, completedGpuTiming
                    );
                }


                // Advance the Benchmark State

                const BenchmarkRunPhase previousPhase = benchmarkController.phase();

                benchmarkController.commitFrame();


                if (previousPhase == BenchmarkRunPhase::Warmup &&
                    benchmarkController.phase() == BenchmarkRunPhase::Measurement) {

                    std::cout << "[Benchmark] Warmup Completed"
                            << " | Frames " << benchmarkController.warmupFramesCompleted()
                            << '\n';

                    std::cout << "[Benchmark] Measurement Started"
                            << '\n';
                }


                if (benchmarkController.isCompleted()) {
                    std::cout << "[Benchmark] Measurement Completed"
                            << " | Frames " << benchmarkController.measurementFramesCompleted()
                            << '\n';

                    std::cout << "[Benchmark] Total Frames "
                            << benchmarkController.totalFramesCompleted()
                            << '\n';

                    window_.requestClose();
                }

                continue;
            }


            // Normal Mode: CPU and Geometry Diagnostics Every 60 Frames

            if (frameIndex % 60 == 0) {
                std::cout << "[Benchmark][CPU]"
                        << " | Frame " << frameIndex
                        << " | Wall " << cpuTimings.frameWallMs << " ms"
                        << " | Events " << cpuTimings.eventPollMs << " ms"
                        << " | Input " << cpuTimings.inputMs << " ms"
                        << " | Update " << cpuTimings.updateMs << " ms"
                        << '\n';

                std::cout << "[Benchmark][Render]"
                        << " | Meshing " << cpuTimings.meshRebuildMs << " ms"
                        << " | Assembly " << cpuTimings.frameAssemblyMs << " ms"
                        << " | Work " << cpuTimings.rendererWorkMs << " ms"
                        << " | Present " << cpuTimings.presentMs << " ms"
                        << " | Backend+Present " << backendCallWallMs << " ms"
                        << '\n';

                std::cout << "[Benchmark][Geometry]"
                        << " | Chunks " << renderStatistics.residentChunkCount
                        << " | Draws " << renderStatistics.meshDrawsSubmitted
                        << " | Vertices " << renderStatistics.meshVerticesReferenced
                        << " | Indices " << renderStatistics.meshIndicesSubmitted
                        << " | Triangles " << renderStatistics.trianglesSubmitted
                        << '\n';
            }


            // Normal Mode: Resource Diagnostics

            const bool resourceEvent =
                renderStatistics.meshRebuildCount > 0 ||
                renderStatistics.gpuMeshUploadCount > 0 ||
                renderStatistics.gpuMeshReleaseCount > 0;

            if (resourceEvent || frameIndex % 60 == 0) {
                std::cout << "[Benchmark][Resources]"
                        << " | Frame " << frameIndex
                        << " | MeshRebuild " << renderStatistics.meshRebuildCount
                        << " | GPUUploads " << renderStatistics.gpuMeshUploadCount
                        << " | GPUReleases " << renderStatistics.gpuMeshReleaseCount
                        << '\n';
            }


            // Normal Mode: Report Asynchronous GPU Results

            if (gpuResultAvailable &&
                completedGpuTiming.state == BenchmarkGpuTimingState::Available &&
                completedGpuFrameId % 60 == 0) {

                std::cout << "[Benchmark][GPU]"
                        << " | Original Frame " << completedGpuFrameId
                        << " | GPU Render " << completedGpuTiming.elapsedMs << " ms"
                        << '\n';
            }
        }


        // Finalize Completed Benchmark

        if (benchmarkMode && benchmarkController.isCompleted()) {
            if (!benchmarkCollector.allCpuFramesRecorded())
                throw std::runtime_error("Benchmark CPU sample collection is incomplete");


            // Drain Outstanding GPU Queries After Measurement

            if (backend_ == RenderBackend::OpenGL &&
                benchmarkCollector.gpuUnresolvedCount() > 0) {

                const auto deadline = std::chrono::steady_clock::now() +
                                    std::chrono::milliseconds(250);

                while (benchmarkCollector.gpuUnresolvedCount() > 0 &&
                    std::chrono::steady_clock::now() < deadline) {

                    std::uint64_t completedFrameId = 0;
                    BenchmarkGpuTiming completedTiming{};

                    if (renderBackendDriver_->tryPopGpuTiming(completedFrameId, completedTiming)) {
                        benchmarkCollector.attachGpuResult(completedFrameId, completedTiming);
                    } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
            }


            // Collection Summary

            const std::vector<BenchmarkFrameSample>& samples = benchmarkCollector.samples();

            std::cout << "[Benchmark][Collection]"
                    << " | CPU Samples " << samples.size()
                    << " / " << benchmarkController.configuration().measurementFrameCount
                    << '\n';


            if (!samples.empty()) {
                std::cout << "[Benchmark][Collection]"
                        << " | First Frame " << samples.front().frameId
                        << " | Last Frame " << samples.back().frameId
                        << '\n';
            }


            if (backend_ == RenderBackend::OpenGL) {
                std::cout << "[Benchmark][Collection]"
                        << " | GPU Available " << benchmarkCollector.gpuAvailableCount()
                        << " | GPU Unresolved " << benchmarkCollector.gpuUnresolvedCount()
                        << '\n';

                if (benchmarkCollector.gpuUnresolvedCount() > 0) {
                    std::cout << "[Benchmark][Warning] Some GPU results were not collected"
                            << '\n';
                }
            } else {
                std::cout << "[Benchmark][Collection]"
                        << " | GPU NotApplicable (Software Backend)"
                        << '\n';
            }


            std::cout << "[Benchmark] Frame Sample Collection Completed"
                    << '\n';
        }


        if (benchmarkMode && !benchmarkController.isCompleted()) {
            std::cout << "[Benchmark] Run Aborted"
                    << " | Completed Frames " << benchmarkController.totalFramesCompleted()
                    << '\n';
        }

        std::cout << "[Benchmark] Frame Sample Collection Completed" << '\n';
        if (benchmarkMode) {
            BenchmarkSummaryPrinter::print(std::cout, benchmarkCollector);

            const auto csvPath = BenchmarkCsvWriter::writeToDirectory(
                benchmarkCollector, "benchmark_results"
            );

            std::cout << "[Benchmark][CSV] Saved " << csvPath.string()
                    << " | Frames " << benchmarkCollector.samples().size()
                    << '\n';
        }

        std::cout << "Voxel Engine Shutdown" << '\n';
    }

}