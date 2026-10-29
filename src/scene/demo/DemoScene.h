#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <map>

#include "assets/AssetManager.h"
#include "scene/Scene.h"
#include "gameplay/core/GameplaySession.h"
#include "gameplay/items/InventoryCursor.h"
#include "gameplay/block/OrientedBlocks.h"
#include "scene/object/SceneObjectId.h"
#include "scene/camera/Camera.h"
#include "scene/camera/CameraController.h"
#include "scene/voxel/VoxelWorld.h"

#include "voxel/generation/TerrainGenerationConfig.h"
#include "voxel/streaming/ChunkStreamingController.h"

namespace ve
{
    enum class DemoTerrainMode {
        Fixed,
        Procedural
    };

    class DemoScene final : public Scene {
        private:
            AssetManager& assets_;
            Camera camera_;
            CameraController controller_;
            GameplaySession gameplay_{};
            InventoryCursor inventoryCursor_{};
            bool inventoryOpen_ = false;
            OrientedBlocks orientedBlocks_{};
            bool orientationSaveWritable_ = true;
            bool inventorySaveWritable_ = true;
            bool playerSaveWritable_ = true;
            bool worldTimeSaveWritable_ = true;
            bool sunShadowsEnabled_ = false;
            SceneObjectId sunObjectId_{};
            int inventoryPointerX_ = -1;
            int inventoryPointerY_ = -1;
            std::map<std::uint64_t, SceneObjectId> dropVisuals_{};
            VoxelWorld voxel_;

            std::optional<ChunkStreamingController> streaming_{};
            DemoTerrainMode terrainMode_ = DemoTerrainMode::Fixed;
            WorldSeed worldSeed_ = 1337;

            std::uint64_t cubeMeshId_ = 0;
            std::uint64_t defaultMaterialId_ = 0;
            std::uint64_t textureId_ = 0;
            std::optional<VoxelRaycastHit> selectedBlock_;

            std::map<BlockMaterialKey, std::uint64_t> voxelMaterials_;
            std::array<std::map<BlockMaterialKey, std::uint64_t>, 3> voxelTextureSets_{};
            std::array<bool, 3> loadedTextureSets_{ true, false, false };
            std::uint8_t activeVoxelTextureSet_ = 0;

            float blockReach_ = 6.0f;
            bool chunkRemovalDemoEnabled_ = false;
            float editElapsed_ = 0.0f;
            bool pitOpen_ = false;
            ShadingModel shadingModel_ = ShadingModel::BlinnPhong;


        public:
            explicit DemoScene( AssetManager& assets, DemoTerrainMode terrainMode = DemoTerrainMode::Fixed,
                                WorldSeed worldSeed = 1337 );

            ~DemoScene() override;

            void resetCamera();
            void clearSelection();
            void disableChunkRemovalDemo();
            void update( float deltaTime, bool cursorCaptured, const MouseDelta& delta,
                         const InputActions& actions ) override;
            bool inventoryOpen() const;
            bool gameplayHudWithoutCapture() const override;
            void appendGameplayHud( UIFrameData& data, int width, int height ) const override;
            GameplaySession& gameplay();
            const GameplaySession& gameplay() const;
            Camera& camera() override;
            const Camera& camera() const override;

            ToneMappingSettings toneMappingSettings() const override;
            ShadingModel shadingModel() const override;
            std::uint8_t voxelTextureSetIndex() const override;
            std::uint64_t voxelMaterialId() const override;
            std::uint64_t voxelMaterialId( BlockMaterialKey material ) const override;
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
            void populateProceduralWorld();
            void updateBlockSelection();
            void updateSunlight();
    };
}
