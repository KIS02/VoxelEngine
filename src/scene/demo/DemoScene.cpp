#include "scene/demo/DemoScene.h"

#include <iostream>
#include <stdexcept>

#include "scene/demo/DemoChunkGenerator.h"
#include "assets/geometry/primitives/CubeMeshGenerator.h"
#include "assets/texture/UVTestTexture.h"
#include "gameplay/interaction/BlockInteractor.h"
#include "assets/block/BlockName.h"


namespace ve
{
    namespace
    {
        bool sameBlockPosition( const WorldBlockPosition& a, const WorldBlockPosition& b ) {
            return a.x == b.x && a.y == b.y && a.z == b.z;
        }
    }


    DemoScene::DemoScene( AssetManager& assets )
        : assets_(assets), voxel_(assets.meshIds()) {
        textureId_ = assets_.addTexture(createUVTestTexture());
        defaultMaterialId_ = assets_.addMaterial(textureId_);
        cubeMeshId_ = assets_.addMesh(CubeMeshGenerator::create(1.0f));

        SceneObject cube{};
        cube.position = Vec3{ 0.0f, 7.0f, 0.0f };
        cube.rotationDegrees = Vec3{ 0.0f, 30.0f, 0.0f };
        cube.meshAssetId = cubeMeshId_;
        cube.materialAssetId = defaultMaterialId_;
        objects().create(cube);

        resetCamera();
        controller_.setMoveSpeed(8.0f);
        controller_.setMouseSensitivity(0.1f);
        populateDemoWorld();
        voxel_.rebuildDirtyMeshes();
    }

    void DemoScene::resetCamera() {
        camera_.setPosition(Vec3{ 8.5f, 6.5f, 10.5f });
        camera_.lookAt(Vec3{ 8.5f, 2.5f, 8.5f });
    }

    void DemoScene::clearSelection() {
        selectedBlock_.reset();
    }

    void DemoScene::disableChunkRemovalDemo() {
        chunkRemovalDemoEnabled_ = false;
    }

    void DemoScene::populateDemoWorld() {
        for (int z = -1; z <= 0; ++z) {
            for (int x = -1; x <= 0; ++x) {
                const ChunkPosition position{ x, 0, z };

                Chunk& chunk = voxel_.blocks().getOrCreateChunk(position);

                chunk = createDemoChunk();

                voxel_.blocks().markChunkDirty(position);
            }
        }
    }


    void DemoScene::updateBlockSelection() {
        const std::optional<VoxelRaycastHit> hit = VoxelRaycaster::cast(
            voxel_.blocks(),
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


    void DemoScene::update( float deltaTime, bool cursorCaptured, const MouseDelta& mouseDelta,
                            const InputActions& actions ) {
        // Camera Movement and Mouse Look

        if (cursorCaptured) {
            

            controller_.updateLook(camera_, mouseDelta);


            controller_.update(camera_, actions.movement, deltaTime);
        }


        // Break or Place a Block Using the Current Camera Direction

        if (cursorCaptured) {
            if (actions.breakBlock) {
                const std::optional<VoxelRaycastHit> broken = BlockInteractor::breakBlock(
                    voxel_.blocks(),
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
                            << " | Dirty Chunks: " << voxel_.blocks().dirtyChunkCount()
                            << std::endl;
                }
            } else if (actions.placeBlock) {
                const std::optional<BlockPlacementResult> placed = BlockInteractor::placeBlock(
                    voxel_.blocks(),
                    camera_.position(),
                    camera_.forward(),
                    blockReach_,
                    BlockId::Dirt
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
                            << " | Dirty Chunks: " << voxel_.blocks().dirtyChunkCount()
                            << std::endl;
                }
            }
        }


        // Input actions are frame-local values and require no manual reset.


        // Existing Optional Chunk Removal Demo

        if (chunkRemovalDemoEnabled_ && !pitOpen_) {
            editElapsed_ += deltaTime;


            if (editElapsed_ >= 3.0f) {
                const ChunkPosition targetPosition{ 0, 0, 0 };

                if (!voxel_.removeChunk(targetPosition)) {
                    throw std::runtime_error("Target Chunk does not exist");
                }


                

                pitOpen_ = true;


                std::cout << "[VoxelStorage] Chunk (0, 0, 0) REMOVED"
                        << " | Remaining Chunks: " << voxel_.blocks().chunkCount()
                        << " | Dirty Chunks: " << voxel_.blocks().dirtyChunkCount()
                        << std::endl;
            }
        }


        // Refresh the Selected Block After All VoxelStorage Changes

        updateBlockSelection();
    }

    std::uint64_t DemoScene::rebuildDirtyMeshes() {
        return voxel_.rebuildDirtyMeshes();
    }

    const ChunkMeshCache& DemoScene::chunkMeshes() const {
        return voxel_.meshes();
    }

    Camera& DemoScene::camera() {
        return camera_;
    }

    const Camera& DemoScene::camera() const {
        return camera_;
    }
    const CameraController& DemoScene::cameraController() const {
        return controller_;
    }
    std::uint64_t DemoScene::voxelMaterialId() const {
        return defaultMaterialId_;
    }

    VoxelWorld* DemoScene::voxelWorld() {
        return &voxel_;
    }

    const VoxelWorld* DemoScene::voxelWorld() const {
        return &voxel_;
    }

    const VoxelStorage& DemoScene::world() const {
        return voxel_.blocks();
    }
    const MeshData& DemoScene::cubeMesh() const {
        return assets_.mesh(cubeMeshId_);
    }
    const MaterialData& DemoScene::defaultMaterial() const {
        return assets_.material(defaultMaterialId_);
    }
    const Texture2D& DemoScene::texture() const {
        return assets_.texture(textureId_);
    }
    std::optional<VoxelRaycastHit> DemoScene::selectedBlock() const {
        return selectedBlock_;
    }
    std::uint64_t DemoScene::cubeMeshId() const {
        return cubeMeshId_;
    }
    float DemoScene::blockReach() const {
        return blockReach_;
    }
    bool DemoScene::chunkRemovalDemoEnabled() const {
        return chunkRemovalDemoEnabled_;
    }
}
