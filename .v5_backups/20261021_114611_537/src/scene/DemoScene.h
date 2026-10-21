#pragma once

#include <cstdint>
#include <optional>

#include "assets/material/MaterialData.h"
#include "assets/mesh/MeshData.h"
#include "assets/texture/Texture2D.h"
#include "input/ApplicationInput.h"
#include "input/MouseTracker.h"
#include "assets/mesh/MeshIdGenerator.h"
#include "scene/Camera.h"
#include "scene/CameraController.h"
#include "world/VoxelRaycaster.h"
#include "world/World.h"
#include "world/meshing/ChunkMeshManager.h"


namespace ve
{
    class DemoScene {
        private:
            Camera camera_;
            CameraController cameraController_;
            MeshData cubeMesh_;
            World world_;
            MeshIdGenerator meshIdGenerator_;
            ChunkMeshManager chunkMeshManager_;
            std::uint64_t cubeMeshId_ = 0;
            Texture2D uvTexture_;
            MaterialData defaultMaterial_{};
            std::optional<VoxelRaycastHit> selectedBlock_;
            float blockReach_ = 6.0f;
            bool chunkRemovalDemoEnabled_ = false;
            float editElapsed_ = 0.0f;
            bool pitOpen_ = false;


        public:
            DemoScene();

            DemoScene( const DemoScene& ) = delete;
            DemoScene& operator=( const DemoScene& ) = delete;
            DemoScene( DemoScene&& ) = delete;
            DemoScene& operator=( DemoScene&& ) = delete;

            void resetCamera();
            void clearSelection();
            void disableChunkRemovalDemo();
            void update( float deltaTime, bool cursorCaptured, const MouseDelta& mouseDelta,
                         const InputActions& actions );
            std::uint64_t rebuildDirtyMeshes();
            const ChunkMeshManager& chunkMeshes() const;

            const Camera& camera() const;
            const CameraController& cameraController() const;
            const World& world() const;
            const MeshData& cubeMesh() const;
            const MaterialData& defaultMaterial() const;
            const Texture2D& texture() const;
            const std::optional<VoxelRaycastHit>& selectedBlock() const;
            std::uint64_t cubeMeshId() const;
            float blockReach() const;
            bool chunkRemovalDemoEnabled() const;

        private:
            void populateDemoWorld();
            void updateBlockSelection();
    };
}
