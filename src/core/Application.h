
#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "benchmark/BenchmarkTypes.h"

#include "core/Timer.h"
#include "core/Window.h"


#include "platform/windows/Win32FramePresenter.h"

#include "render/backends/software/DepthBuffer.h"
#include "render/backends/software/FrameBuffer.h"
#include "render/api/IRenderBackend.h"
#include "assets/mesh/MeshData.h"
#include "render/MeshIDGenerator.h"
#include "render/backends/software/Rasterizer.h"
#include "render/api/RenderBackend.h"
#include "render/api/RenderFrame.h"
#include "assets/material/MaterialData.h"
#include "assets/texture/Texture2D.h"
#include "render/backends/software/Viewport.h"

#include "render/backends/opengl/OpenGLRenderer.h"

#include "scene/Camera.h"
#include "scene/CameraController.h"

#include "world/VoxelRaycaster.h"
#include "world/World.h"
#include "world/meshing/ChunkMeshManager.h"

#include "debug/DebugOverlayTelemetry.h"

namespace ve
{
    class Application {
        private:
            // Core

            RenderBackend backend_;

            Window window_;
            Timer timer_;

            Camera camera_;
            CameraController cameraController_;


            // Software Rendering

            FrameBuffer frameBuffer_;
            DepthBuffer depthBuffer_;

            Rasterizer rasterizer_;
            Viewport viewport_;

            Win32FramePresenter presenter_;


            // World and Mesh Resources

            MeshData cubeMesh_;

            World world_;

            MeshIdGenerator meshIdGenerator_;
            ChunkMeshManager chunkMeshManager_;
            std::uint64_t cubeMeshId_ = 0;
            Texture2D uvTexture_;
            MaterialData defaultMaterial_{};


            // OpenGL Rendering

            OpenGLRenderer openGLRenderer_;

            std::unique_ptr<IRenderBackend> renderBackendDriver_;


            // Input

            bool escapeKeyWasPressed_ = false;

            bool leftMouseButtonWasPressed_ = false;
            bool rightMouseButtonWasPressed_ = false;

            bool breakBlockRequested_ = false;
            bool placeBlockRequested_ = false;
            
            // Debug Overlay

            DebugOverlayTelemetry debugTelemetry_;

            bool debugOverlayEnabled_ = false;
            bool f3KeyWasPressed_ = false;

            // Block Selection

            std::optional<VoxelRaycastHit> selectedBlock_;
            float blockReach_ = 6.0f;


            // Optional Chunk Removal Demo

            bool chunkRemovalDemoEnabled_ = false;

            float editElapsed_ = 0.0f;
            bool pitOpen_ = false;


        public:
            explicit Application( RenderBackend backend );

            void run( bool benchmarkMode = false );


        private:
            void populateDemoWorld();

            void updateBlockSelection();

            RenderFrame createDemoFrame( int width, int height ) const;

            void processInput();
            void update( float deltaTime );

            void render( 
                BenchmarkCpuTimings& cpuTimings, 
                double& backendCallWallMs,
                std::uint64_t frameId, 
                BenchmarkRenderStatistics& renderStatistics );
    };
}
