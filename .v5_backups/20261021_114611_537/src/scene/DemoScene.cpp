#include "scene/DemoScene.h"

#include <iostream>
#include <stdexcept>

#include "demo/ChunkDemoFactory.h"
#include "geometry/CubeMeshFactory.h"
#include "render/texture/UVTestTexture.h"
#include "world/BlockInteractor.h"
#include "world/BlockTypeName.h"


namespace ve
{
    namespace
    {
        bool sameBlockPosition( const WorldBlockPosition& a, const WorldBlockPosition& b ) {
            return a.x == b.x && a.y == b.y && a.z == b.z;
        }
    }


    DemoScene::DemoScene()
        : cubeMesh_(CubeMeshFactory::create(1.0f)),
          chunkMeshManager_(meshIdGenerator_), uvTexture_(createUVTestTexture()) {
        defaultMaterial_.texture = &uvTexture_;
        resetCamera();
        cameraController_.setMoveSpeed(8.0f);
        cameraController_.setMouseSensitivity(0.1f);
        populateDemoWorld();
        chunkMeshManager_.rebuildDirtyMeshes(world_);
        cubeMeshId_ = meshIdGenerator_.generate();
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

                Chunk& chunk = world_.getOrCreateChunk(position);

                chunk = createDemoChunk();

                world_.markChunkDirty(position);
            }
        }
    }


    void DemoScene::updateBlockSelection() {
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


    void DemoScene::update( float deltaTime, bool cursorCaptured, const MouseDelta& mouseDelta,
                            const InputActions& actions ) {
        // Camera Movement and Mouse Look

        if (cursorCaptured) {
            

            cameraController_.updateLook(camera_, mouseDelta);


            cameraController_.update(camera_, actions.movement, deltaTime);
        }


        // Break or Place a Block Using the Current Camera Direction

        if (cursorCaptured) {
            if (actions.breakBlock) {
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
            } else if (actions.placeBlock) {
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


        // Input actions are frame-local values and require no manual reset.


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

    std::uint64_t DemoScene::rebuildDirtyMeshes() {
        return chunkMeshManager_.rebuildDirtyMeshes(world_);
    }

    const ChunkMeshManager& DemoScene::chunkMeshes() const {
        return chunkMeshManager_;
    }

    const Camera& DemoScene::camera() const {
        return camera_;
    }
    const CameraController& DemoScene::cameraController() const {
        return cameraController_;
    }
    const World& DemoScene::world() const {
        return world_;
    }
    const MeshData& DemoScene::cubeMesh() const {
        return cubeMesh_;
    }
    const MaterialData& DemoScene::defaultMaterial() const {
        return defaultMaterial_;
    }
    const Texture2D& DemoScene::texture() const {
        return uvTexture_;
    }
    const std::optional<VoxelRaycastHit>& DemoScene::selectedBlock() const {
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
