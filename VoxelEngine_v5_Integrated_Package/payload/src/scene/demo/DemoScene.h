#pragma once

#include <cstdint>
#include <optional>

#include "assets/AssetManager.h"
#include "scene/Scene.h"
#include "scene/camera/Camera.h"
#include "scene/camera/CameraController.h"
#include "scene/voxel/VoxelWorld.h"


namespace ve
{
    class DemoScene final : public Scene {
        private:
            AssetManager& assets_;
            Camera camera_;
            CameraController controller_;
            VoxelWorld voxel_;
            std::uint64_t cubeMeshId_ = 0;
            std::uint64_t defaultMaterialId_ = 0;
            std::uint64_t textureId_ = 0;
            std::optional<VoxelRaycastHit> selectedBlock_;
            float blockReach_ = 6.0f;
            bool chunkRemovalDemoEnabled_ = false;
            float editElapsed_ = 0.0f;
            bool pitOpen_ = false;

        public:
            explicit DemoScene( AssetManager& assets );
            void resetCamera();
            void clearSelection();
            void disableChunkRemovalDemo();
            void update( float deltaTime, bool cursorCaptured, const MouseDelta& delta,
                         const InputActions& actions ) override;
            Camera& camera() override;
            const Camera& camera() const override;
            std::uint64_t voxelMaterialId() const override;
            VoxelWorld* voxelWorld() override;
            const VoxelWorld* voxelWorld() const override;
            std::optional<VoxelRaycastHit> selectedBlock() const override;
            const CameraController& cameraController() const;
            const VoxelStorage& world() const;
            std::uint64_t rebuildDirtyMeshes();
            const ChunkMeshCache& chunkMeshes() const;
            const MeshData& cubeMesh() const;
            const MaterialData& defaultMaterial() const;
            const Texture2D& texture() const;
            std::uint64_t cubeMeshId() const;
            float blockReach() const;
            bool chunkRemovalDemoEnabled() const;

        private:
            void populateDemoWorld();
            void updateBlockSelection();
    };
}
