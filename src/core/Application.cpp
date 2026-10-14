#include "Application.h"

#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "demo/ChunkDemoFactory.h"

#include "math/Mat4.h"
#include "math/Vec3.h"

#include "render/Color.h"

#include "render/mesh/ChunkMeshFactory.h"
#include "render/mesh/CubeMeshFactory.h"
#include "render/texture/UVTestTexture.h"

#include "render/opengl/OpenGLRenderBackend.h"
#include "render/software/SoftwareRenderBackend.h"

#include "world/BlockInteractor.h"



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
          uvTexture_(createUVTestTexture()) {



        // Camera

        camera_.setPosition(Vec3{ 8.5f, 6.5f, 10.5f });
        camera_.lookAt(Vec3{ 8.5f, 2.5f, 8.5f });

        cameraController_.setMoveSpeed(8.0f);
        cameraController_.setMouseSensitivity(0.1f);



        // World

        populateDemoWorld();
        rebuildDirtyMeshes();



        // Rendering Backend

        if (backend_ == RenderBackend::OpenGL) {
            renderBackendDriver_ = std::make_unique<OpenGLRenderBackend>(
                window_, openGLRenderer_, openGLCubeDemo_, cubeMesh_, uvTexture_
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



    void Application::rebuildDirtyMeshes() {
        try {
            world_.processDirtyChunks([this]( const ChunkPosition& position ) {
                if (world_.findChunk(position) == nullptr) {
                    return;
                }



                MeshData newMesh = ChunkMeshFactory::create(world_, position);

                const auto found = chunkMeshes_.find(position);



                // First Mesh Creation

                if (found == chunkMeshes_.end()) {
                    const std::uint64_t meshId = meshIdGenerator_.generate();

                    ChunkMeshEntry entry{
                        std::move(newMesh),
                        1,
                        meshId
                    };

                    chunkMeshes_.emplace(position, std::move(entry));



                    std::cout << "[ChunkMesh] Created ("
                              << position.x << ", "
                              << position.y << ", "
                              << position.z << ")"
                              << " | Mesh ID " << meshId
                              << " | Revision 1"
                              << std::endl;
                }
                else {
                    // Rebuild Existing Mesh

                    if (found->second.revision == std::numeric_limits<std::uint64_t>::max()) {
                        throw std::overflow_error("Chunk Mesh revision overflow");
                    }



                    found->second.mesh = std::move(newMesh);
                    ++found->second.revision;



                    std::cout << "[ChunkMesh] Rebuilt ("
                              << position.x << ", "
                              << position.y << ", "
                              << position.z << ")"
                              << " | Mesh ID " << found->second.id
                              << " | Revision " << found->second.revision
                              << std::endl;
                }
            });
        }
        catch (const std::exception& error) {
            std::cerr << "[ChunkMesh] Rebuild failed: "
                      << error.what()
                      << " | Dirty Chunks Remaining: "
                      << world_.dirtyChunkCount()
                      << std::endl;
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

        for (const auto& [position, entry] : chunkMeshes_) {
            const Vec3 chunkTranslation{
                static_cast<float>(position.x) * static_cast<float>(Chunk::Size),
                static_cast<float>(position.y) * static_cast<float>(Chunk::Size),
                static_cast<float>(position.z) * static_cast<float>(Chunk::Size)
            };

            const Mat4 chunkModel = Mat4::translation(chunkTranslation);

            frame.draws.push_back(RenderDraw{
                &entry.mesh,
                chunkModel,
                entry.revision,
                entry.id
            });
        }



        // Reference Cube

        const Mat4 cubeModel =
            Mat4::translation(Vec3{ 0.0f, 7.0f, 0.0f }) *
            Mat4::rotationY(30.0f * pi / 180.0f);

        frame.draws.push_back(RenderDraw{
            &cubeMesh_,
            cubeModel,
            0,
            0
        });



        return frame;
    }


    void Application::processInput() {
        // Input Requests Are Rebuilt Every Frame

        breakBlockRequested_ = false;
        placeBlockRequested_ = false;



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



                chunkMeshes_.erase(targetPosition);

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

    void Application::render() {
        // Rebuild Dirty Chunk Meshes Before Rendering

        rebuildDirtyMeshes();



        const RenderSize size = renderBackendDriver_->framebufferSize();

        if (size.width <= 0 || size.height <= 0) {
            return;
        }



        const RenderFrame frame = createDemoFrame(size.width, size.height);

        renderBackendDriver_->render(frame);
    }



    void Application::run() {
        std::cout << "Voxel Engine Started" << '\n';



        while (!window_.shouldClose()) {
            window_.pollEvents();

            const float deltaTime = timer_.tick();



            processInput();
            update(deltaTime);
            render();
        }



        std::cout << "Voxel Engine Shutdown" << '\n';
    }
}