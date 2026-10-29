#include "core/Application.h"

#include <memory>

#include "core/Timer.h"
#include "platform/window/Window.h"
#include "assets/AssetManager.h"
#include "scene/SceneManager.h"
#include "scene/camera/Camera.h"
#include "scene/voxel/VoxelWorld.h"
#include "ui/UIManager.h"
#include "input/ApplicationInput.h"
#include "render/host/RenderHost.h"
#include "scene/demo/DemoScene.h"

#include <iostream>
#include <optional>
#include <stdexcept>

#include "benchmark/BenchmarkCpuTimer.h"
#include "benchmark/BenchmarkRenderStatisticsCollector.h"
#include "benchmark/BenchmarkSession.h"
#include "benchmark/LiveBenchmarkReporter.h"
#include "render/frame/FrameBuilder.h"


namespace ve
{
    struct Application::Impl
    {
        RenderBackend backend_;
        Window window_;
        Timer timer_;
        AssetManager assets_;
        SceneManager sceneManager_;
        DemoScene* demo_ = nullptr; // Non-owning; SceneManager owns the Scene.
        RenderHost renderer_;
        ApplicationInput input_;
        InputActions inputActions_{};
        UIManager ui_;


        explicit Impl( RenderBackend backend, bool proceduralTerrain, std::uint64_t seed );
        DemoScene& demo();
        const DemoScene& demo() const;
        void processInput();
        void update( float deltaTime );
        void render( BenchmarkCpuTimings& cpuTimings, double& backendCallWallMs,
                     std::uint64_t frameId, BenchmarkRenderStatistics& renderStatistics );
        void run( bool benchmarkMode );
    };


    Application::Impl::Impl( RenderBackend backend, bool proceduralTerrain, std::uint64_t seed )
        : backend_(backend),
        window_(800, 600, "Voxel Engine", backend == RenderBackend::OpenGL
            ? WindowClientApi::OpenGL : WindowClientApi::None), renderer_(backend, window_) {

        const DemoTerrainMode mode = proceduralTerrain ? DemoTerrainMode::Procedural : DemoTerrainMode::Fixed;

        auto scene = std::make_unique<DemoScene>(assets_, mode, seed);

        demo_ = scene.get();
        if (!sceneManager_.add("demo", std::move(scene)) ||
            !sceneManager_.requestActivation("demo") || !sceneManager_.applyPendingActivation())
            throw std::runtime_error("Unable to activate demo Scene");

        window_.setCursorCaptured(true);
        // Diagnostics

        std::cout << "[Application] Render Backend initialized" << std::endl;

        std::cout << "[VoxelStorage] " << demo().world().chunkCount()
                  << " Chunks created" << std::endl;

        std::cout << "[Camera] Position ("
                  << demo().camera().position().x << ", "
                  << demo().camera().position().y << ", "
                  << demo().camera().position().z << ")"
                  << std::endl;

        std::cout << "[Camera] Movement Speed: "
                  << demo().cameraController().moveSpeed()
                  << " units/second" << std::endl;

        std::cout << "[Camera] Mouse Sensitivity: "
                  << demo().cameraController().mouseSensitivity()
                  << " degrees/pixel" << std::endl;

        std::cout << "[Raycast] Maximum Reach: "
                  << demo().blockReach() << " blocks" << std::endl;

        std::cout << "[Controls] WASD: Move | Space: Up | Left Shift: Down"
                  << std::endl;

        std::cout << "[Controls] P: Toggle Perspective/Orthographic" << std::endl;
        std::cout << "[Controls] G: Creative/Survival | R: Respawn" << std::endl;
        std::cout << "[Controls] Survival: WASD Walk | Space Jump | Left Shift Sprint" << std::endl;
        std::cout << "[Controls] F: Eat selected food | R: Respawn | G: Game Mode" << std::endl;
        std::cout << "[Controls] M: Toggle Blinn-Phong/PBR" << std::endl;
        std::cout << "[Controls] N: Default/Set 1/Set 2 Texture Cycle" << std::endl;
        std::cout << "[Controls] Mouse: Look | Left Click: Break Block"
                  << " | Right Click: Place Dirt"
                  << " | Escape: Release/Exit"
                  << std::endl;

        std::cout << "[Controls] Left Click with Free Cursor: Capture Mouse"
                  << std::endl;

        std::cout << "[Overlay] Crosshair and Selection Outline enabled in FPS mode"
                  << std::endl;

        if (!demo().chunkRemovalDemoEnabled()) {
            std::cout << "[Demo] Automatic Chunk removal disabled"
                      << std::endl;
        }
    }

    DemoScene& Application::Impl::demo() {
        return *demo_;
    }

    const DemoScene& Application::Impl::demo() const {
        return *demo_;
    }

    void Application::Impl::processInput() {
        InputSnapshot snapshot{};
        snapshot.escape = window_.isKeyPressed(Key::Escape);
        snapshot.debugOverlay = window_.isKeyPressed(Key::F3);
        snapshot.toggleProjection = window_.isKeyPressed(Key::P);
        snapshot.toggleGameMode = window_.isKeyPressed(Key::G);
        snapshot.respawn = window_.isKeyPressed(Key::R);
        snapshot.toggleShadingModel = window_.isKeyPressed(Key::M);
        snapshot.cycleTextureSet = window_.isKeyPressed(Key::N);
        snapshot.toggleInventory = window_.isKeyPressed(Key::E);
        snapshot.dropSelected = window_.isKeyPressed(Key::Q);
        snapshot.consumeFood = window_.isKeyPressed(Key::F);
        snapshot.shiftHeld = window_.isKeyPressed(Key::LeftShift);
        const Vec2 pointer = window_.cursorPosition();
        snapshot.cursorX = static_cast<int>(pointer.x);
        snapshot.cursorY = static_cast<int>(pointer.y);
        snapshot.hotbarScroll = window_.takeScrollSteps();
        for (int number = 0; number < 9; ++number) {
            if (window_.isKeyPressed(static_cast<Key>(static_cast<int>(Key::Digit1) + number))) {
                snapshot.hotbarNumber = number;
                break;
            }
        }
        snapshot.leftMouse = window_.isMouseButtonPressed(MouseButton::Left);
        snapshot.rightMouse = window_.isMouseButtonPressed(MouseButton::Right);
        snapshot.movement.forward = window_.isKeyPressed(Key::W);
        snapshot.movement.backward = window_.isKeyPressed(Key::S);
        snapshot.movement.left = window_.isKeyPressed(Key::A);
        snapshot.movement.right = window_.isKeyPressed(Key::D);
        snapshot.movement.up = window_.isKeyPressed(Key::Space);
        snapshot.movement.down = window_.isKeyPressed(Key::LeftShift);
        inputActions_ = input_.process(snapshot, window_.isCursorCaptured(), demo().inventoryOpen());
        if (demo().gameplay().player().alive &&
            (inputActions_.toggleInventory || inputActions_.closeInventory)) {
            if (inputActions_.toggleInventory && !demo().inventoryOpen())
                window_.setCursorCaptured(false);
            else if (demo().inventoryOpen())
                window_.setCursorCaptured(true);
        }

        if (inputActions_.toggleDebugOverlay) {
            ui_.toggleDebug();
            std::cout << "[DebugOverlay] " << (ui_.debugEnabled() ? "Enabled" : "Disabled") << '\n';
        }
        if (inputActions_.releaseCursor) {
            window_.setCursorCaptured(false);
            std::cout << "[Input] Cursor released\n";
        } else if (inputActions_.captureCursor) {
            window_.setCursorCaptured(true);
            std::cout << "[Input] Cursor captured\n";
        }
        if (inputActions_.toggleProjection) {
            Camera& camera = sceneManager_.active()->camera();
            const Camera::ProjectionMode next = camera.projectionMode() == Camera::ProjectionMode::Perspective
                ? Camera::ProjectionMode::Orthographic : Camera::ProjectionMode::Perspective;
            camera.setProjectionMode(next);
            std::cout << "[Camera] Projection: "
                      << (next == Camera::ProjectionMode::Perspective ? "Perspective" : "Orthographic") << '\n';
        }
        if (demo().inventoryOpen() || inputActions_.toggleInventory) {
            inputActions_.captureCursor = false;
            inputActions_.releaseCursor = false;
        }
        if (inputActions_.requestExit)
            window_.requestClose();
    }

    void Application::Impl::update( float deltaTime ) {
        sceneManager_.active()->update(deltaTime, window_.isCursorCaptured(),
                                       window_.mouseDelta(), inputActions_);
        if (demo().inventoryOpen() && window_.isCursorCaptured())
            window_.setCursorCaptured(false);
    }

    void Application::Impl::render( BenchmarkCpuTimings& cpuTimings, double& backendCallWallMs,
                            std::uint64_t frameId, BenchmarkRenderStatistics& renderStatistics ) {
        // Dirty Chunk Mesh Rebuild

        std::uint64_t rebuiltChunkMeshes = 0;

        {
            BenchmarkCpuScope scope(cpuTimings.meshRebuildMs);
            if (VoxelWorld* voxel = sceneManager_.active()->voxelWorld())
                rebuiltChunkMeshes = voxel->rebuildDirtyMeshes();
        }


        const VoxelWorld* voxel = sceneManager_.active()->voxelWorld();
        const std::uint64_t chunkCount = voxel ? static_cast<std::uint64_t>(voxel->blocks().chunkCount()) : 0;
        renderStatistics.residentChunkCount = chunkCount;
        renderStatistics.meshRebuildCount = rebuiltChunkMeshes;


        // RenderFrame Assembly and Geometry Statistics

        std::optional<RenderFrame> frame;

        {
            BenchmarkCpuScope scope(cpuTimings.frameAssemblyMs);

            const RenderSize size = renderer_.framebufferSize();

            if (size.width > 0 && size.height > 0) {
                frame.emplace(FrameBuilder::build(
                    *sceneManager_.active(), assets_, ui_, size.width, size.height,
                    window_.isCursorCaptured(), backend_
                ));

                renderStatistics = BenchmarkRenderStatisticsCollector::collect(
                    *frame, chunkCount
                );

                renderStatistics.meshRebuildCount = rebuiltChunkMeshes;
            }
        }


        if (!frame.has_value())
            return;


        // Backend Rendering and Presentation

        {
            BenchmarkCpuScope scope(backendCallWallMs);

            FrameBuilder::submit(renderer_, *frame, cpuTimings, frameId, &renderStatistics);
        }
    }

    void Application::Impl::run( bool benchmarkMode ) {
        std::cout << "Voxel Engine Started\n";
        BenchmarkSession benchmark{};
        if (benchmarkMode) {
            demo().resetCamera();
            demo().clearSelection();
            demo().disableChunkRemovalDemo();
            inputActions_ = InputActions{};
            window_.setCursorCaptured(false);
            ui_.setDebugEnabled(false);

            const RenderSize size = renderer_.framebufferSize();
            if (size.width != 800 || size.height != 600)
                throw std::runtime_error("Benchmark requires an 800x600 framebuffer");
            if (demo().world().dirtyChunkCount() != 0)
                throw std::runtime_error("Benchmark requires a clean initial VoxelStorage");
            if (backend_ == RenderBackend::OpenGL)
                window_.setSwapInterval(0);

            BenchmarkRunConfiguration configuration{};
            configuration.backend = backend_ == RenderBackend::OpenGL
                ? BenchmarkBackend::OpenGL : BenchmarkBackend::Software;
            configuration.scenarioName = "fixed_demo_world_4_chunks";
#ifdef NDEBUG
            configuration.buildConfiguration = "NDEBUG build";
#else
            configuration.buildConfiguration = "Non-NDEBUG build";
#endif
            configuration.framebufferWidth = size.width;
            configuration.framebufferHeight = size.height;
            configuration.vsyncEnabled = backend_ == RenderBackend::OpenGL && window_.requestedSwapInterval() == 1;
            configuration.crosshairEnabled = false;
            configuration.selectionOutlineEnabled = false;
            const Vec3 position = demo().camera().position();
            const Vec3 forward = demo().camera().forward();
            configuration.cameraPosition = { position.x, position.y, position.z };
            configuration.cameraForward = { forward.x, forward.y, forward.z };
            configuration.warmupFrameCount = 120;
            configuration.measurementFrameCount = 600;
            benchmark.begin(configuration, static_cast<std::uint64_t>(demo().world().chunkCount()),
                            window_.requestedSwapInterval());
        }

        std::uint64_t frameIndex = 0;
        while (!window_.shouldClose()) {
            BenchmarkCpuTimings cpuTimings{};
            BenchmarkRenderStatistics renderStatistics{};
            double backendCallWallMs = 0.0;
            std::uint64_t completedGpuFrameId = 0;
            BenchmarkGpuTiming completedGpuTiming{};
            bool gpuResultAvailable = false;

            {
                BenchmarkCpuScope frameScope(cpuTimings.frameWallMs);
                {
                    BenchmarkCpuScope eventScope(cpuTimings.eventPollMs);
                    window_.pollEvents();
                }
                if (benchmarkMode && window_.shouldClose())
                    break;
                const float deltaTime = timer_.tick();
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
                ++frameIndex;
                render(cpuTimings, backendCallWallMs, frameIndex, renderStatistics);
                gpuResultAvailable = renderer_.tryPopGpuTiming(completedGpuFrameId, completedGpuTiming);
            }

            if (!benchmarkMode) {
                ui_.recordFrame(cpuTimings, renderStatistics);
                if (gpuResultAvailable)
                    ui_.observeGpuTiming(completedGpuTiming);
                LiveBenchmarkReporter::report(frameIndex, cpuTimings, renderStatistics, backendCallWallMs,
                                              gpuResultAvailable, completedGpuFrameId, completedGpuTiming);
                continue;
            }

            const bool completed = benchmark.recordFrame(
                frameIndex, cpuTimings, renderStatistics, renderer_.framebufferSize(),
                gpuResultAvailable, completedGpuFrameId, completedGpuTiming
            );
            if (completed)
                window_.requestClose();
        }

        if (benchmarkMode)
            benchmark.finish([this]( std::uint64_t& frameId, BenchmarkGpuTiming& timing ) {
                return renderer_.tryPopGpuTiming(frameId, timing);
            });
        std::cout << "Voxel Engine Shutdown\n";
    }


    Application::Application( RenderBackend backend, bool proceduralTerrain, std::uint64_t seed )
        : impl_(std::make_unique<Impl>(backend, proceduralTerrain, seed)) { }
        
    Application::~Application() = default;

    void Application::run( bool benchmarkMode ) {
        impl_->run(benchmarkMode);
    }
}
