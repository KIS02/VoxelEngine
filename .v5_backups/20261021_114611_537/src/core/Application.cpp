#include "core/Application.h"

#include <memory>

#include "core/Timer.h"
#include "core/Window.h"
#include "debug/DebugOverlayTelemetry.h"
#include "input/ApplicationInput.h"
#include "render/RenderHost.h"
#include "scene/DemoScene.h"

#include <iostream>
#include <optional>
#include <stdexcept>

#include "benchmark/BenchmarkCpuTimer.h"
#include "benchmark/BenchmarkRenderStatisticsCollector.h"
#include "benchmark/BenchmarkSession.h"
#include "benchmark/LiveBenchmarkReporter.h"
#include "scene/DemoFrameBuilder.h"


namespace ve
{
    struct Application::Impl
    {
        RenderBackend backend_;
        Window window_;
        Timer timer_;
        DemoScene scene_;
        RenderHost renderer_;
        ApplicationInput input_;
        InputActions inputActions_{};
        DebugOverlayTelemetry debugTelemetry_;
        bool debugOverlayEnabled_ = false;


        explicit Impl( RenderBackend backend );
        void processInput();
        void update( float deltaTime );
        void render( BenchmarkCpuTimings& cpuTimings, double& backendCallWallMs,
                     std::uint64_t frameId, BenchmarkRenderStatistics& renderStatistics );
        void run( bool benchmarkMode );
    };


    Application::Impl::Impl( RenderBackend backend )
        : backend_(backend),
          window_(800, 600, "Voxel Engine", backend == RenderBackend::OpenGL
              ? WindowClientApi::OpenGL : WindowClientApi::None),
          renderer_(backend, window_, scene_.texture()) {
        window_.setCursorCaptured(true);
        // Diagnostics

        std::cout << "[Application] Render Backend initialized" << std::endl;

        std::cout << "[World] " << scene_.world().chunkCount()
                  << " Chunks created" << std::endl;

        std::cout << "[Camera] Position ("
                  << scene_.camera().position().x << ", "
                  << scene_.camera().position().y << ", "
                  << scene_.camera().position().z << ")"
                  << std::endl;

        std::cout << "[Camera] Movement Speed: "
                  << scene_.cameraController().moveSpeed()
                  << " units/second" << std::endl;

        std::cout << "[Camera] Mouse Sensitivity: "
                  << scene_.cameraController().mouseSensitivity()
                  << " degrees/pixel" << std::endl;

        std::cout << "[Raycast] Maximum Reach: "
                  << scene_.blockReach() << " blocks" << std::endl;

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

        if (!scene_.chunkRemovalDemoEnabled()) {
            std::cout << "[Demo] Automatic Chunk removal disabled"
                      << std::endl;
        }
    }

    void Application::Impl::processInput() {
        InputSnapshot snapshot{};
        snapshot.escape = window_.isKeyPressed(Key::Escape);
        snapshot.debugOverlay = window_.isKeyPressed(Key::F3);
        snapshot.leftMouse = window_.isMouseButtonPressed(MouseButton::Left);
        snapshot.rightMouse = window_.isMouseButtonPressed(MouseButton::Right);
        snapshot.movement.forward = window_.isKeyPressed(Key::W);
        snapshot.movement.backward = window_.isKeyPressed(Key::S);
        snapshot.movement.left = window_.isKeyPressed(Key::A);
        snapshot.movement.right = window_.isKeyPressed(Key::D);
        snapshot.movement.up = window_.isKeyPressed(Key::Space);
        snapshot.movement.down = window_.isKeyPressed(Key::LeftShift);
        inputActions_ = input_.process(snapshot, window_.isCursorCaptured());

        if (inputActions_.toggleDebugOverlay) {
            debugOverlayEnabled_ = !debugOverlayEnabled_;
            std::cout << "[DebugOverlay] " << (debugOverlayEnabled_ ? "Enabled" : "Disabled") << '\n';
        }
        if (inputActions_.releaseCursor) {
            window_.setCursorCaptured(false);
            std::cout << "[Input] Cursor released\n";
        } else if (inputActions_.captureCursor) {
            window_.setCursorCaptured(true);
            std::cout << "[Input] Cursor captured\n";
        }
        if (inputActions_.requestExit)
            window_.requestClose();
    }

    void Application::Impl::update( float deltaTime ) {
        scene_.update(deltaTime, window_.isCursorCaptured(), window_.mouseDelta(), inputActions_);
    }

    void Application::Impl::render( BenchmarkCpuTimings& cpuTimings, double& backendCallWallMs,
                            std::uint64_t frameId, BenchmarkRenderStatistics& renderStatistics ) {
        // Dirty Chunk Mesh Rebuild

        std::uint64_t rebuiltChunkMeshes = 0;

        {
            BenchmarkCpuScope scope(cpuTimings.meshRebuildMs);
            rebuiltChunkMeshes = scene_.rebuildDirtyMeshes();
        }


        renderStatistics.residentChunkCount = scene_.world().chunkCount();
        renderStatistics.meshRebuildCount = rebuiltChunkMeshes;


        // RenderFrame Assembly and Geometry Statistics

        std::optional<RenderFrame> frame;

        {
            BenchmarkCpuScope scope(cpuTimings.frameAssemblyMs);

            const RenderSize size = renderer_.framebufferSize();

            if (size.width > 0 && size.height > 0) {
                frame.emplace(DemoFrameBuilder::build(
                    scene_, size.width, size.height, window_.isCursorCaptured(),
                    debugOverlayEnabled_, backend_, debugTelemetry_
                ));

                renderStatistics = BenchmarkRenderStatisticsCollector::collect(
                    *frame, static_cast<std::uint64_t>(scene_.world().chunkCount())
                );

                renderStatistics.meshRebuildCount = rebuiltChunkMeshes;
            }
        }


        if (!frame.has_value())
            return;


        // Backend Rendering and Presentation

        {
            BenchmarkCpuScope scope(backendCallWallMs);

            renderer_.renderMeasured(
                *frame, cpuTimings, frameId, &renderStatistics
            );
        }
    }

    void Application::Impl::run( bool benchmarkMode ) {
        std::cout << "Voxel Engine Started\n";
        BenchmarkSession benchmark{};
        if (benchmarkMode) {
            scene_.resetCamera();
            scene_.clearSelection();
            scene_.disableChunkRemovalDemo();
            inputActions_ = InputActions{};
            window_.setCursorCaptured(false);
            debugOverlayEnabled_ = false;

            const RenderSize size = renderer_.framebufferSize();
            if (size.width != 800 || size.height != 600)
                throw std::runtime_error("Benchmark requires an 800x600 framebuffer");
            if (scene_.world().dirtyChunkCount() != 0)
                throw std::runtime_error("Benchmark requires a clean initial World");
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
            const Vec3 position = scene_.camera().position();
            const Vec3 forward = scene_.camera().forward();
            configuration.cameraPosition = { position.x, position.y, position.z };
            configuration.cameraForward = { forward.x, forward.y, forward.z };
            configuration.warmupFrameCount = 120;
            configuration.measurementFrameCount = 600;
            benchmark.begin(configuration, static_cast<std::uint64_t>(scene_.world().chunkCount()),
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
                debugTelemetry_.recordFrame(cpuTimings, renderStatistics);
                if (gpuResultAvailable)
                    debugTelemetry_.observeGpuTiming(completedGpuTiming);
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


    Application::Application( RenderBackend backend )
        : impl_(std::make_unique<Impl>(backend)) { }

    Application::~Application() = default;

    void Application::run( bool benchmarkMode ) {
        impl_->run(benchmarkMode);
    }
}
