#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>

#include "core/Timer.h"
#include "core/Window.h"

#include "demo/OpenGLCubeDemo.h"

#include "platform/windows/Win32FramePresenter.h"

#include "render/DepthBuffer.h"
#include "render/FrameBuffer.h"
#include "render/IRenderBackend.h"
#include "render/MeshData.h"
#include "render/MeshIdGenerator.h"
#include "render/Rasterizer.h"
#include "render/RenderBackend.h"
#include "render/RenderFrame.h"
#include "render/Texture2D.h"
#include "render/Viewport.h"

#include "render/opengl/OpenGLRenderer.h"

#include "scene/Camera.h"
#include "scene/CameraController.h"

#include "world/VoxelRaycaster.h"
#include "world/World.h"



namespace ve
{
    class Application {
        private:
            struct ChunkMeshEntry
            {
                MeshData mesh;
                std::uint64_t revision = 1;
                std::uint64_t id = 0;
            };



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

            std::map<ChunkPosition, ChunkMeshEntry, ChunkPositionLess> chunkMeshes_;

            MeshIdGenerator meshIdGenerator_;
            Texture2D uvTexture_;



            // OpenGL Rendering

            OpenGLRenderer openGLRenderer_;
            OpenGLCubeDemo openGLCubeDemo_;

            std::unique_ptr<IRenderBackend> renderBackendDriver_;



            // Input

            bool escapeKeyWasPressed_ = false;

            bool leftMouseButtonWasPressed_ = false;
            bool rightMouseButtonWasPressed_ = false;

            bool breakBlockRequested_ = false;
            bool placeBlockRequested_ = false;



            // Block Selection

            std::optional<VoxelRaycastHit> selectedBlock_;
            float blockReach_ = 6.0f;



            // Optional Chunk Removal Demo

            bool chunkRemovalDemoEnabled_ = false;

            float editElapsed_ = 0.0f;
            bool pitOpen_ = false;


        public:
            explicit Application( RenderBackend backend );

            void run();


        private:
            void populateDemoWorld();
            void rebuildDirtyMeshes();

            void updateBlockSelection();

            RenderFrame createDemoFrame( int width, int height ) const;

            void processInput();
            void update( float deltaTime );
            void render();
    };
}