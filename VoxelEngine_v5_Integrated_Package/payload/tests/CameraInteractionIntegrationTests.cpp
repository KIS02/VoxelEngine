#include <cmath>
#include <cstddef>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <set>
#include <stdexcept>

#include "math/Vec3.h"
#include "assets/mesh/MeshData.h"
#include "voxel/chunk/ChunkMeshBuilder.h"
#include "scene/camera/Camera.h"
#include "scene/camera/CameraController.h"
#include "gameplay/interaction/BlockInteractor.h"
#include "voxel/block/BlockId.h"
#include "voxel/chunk/VoxelStorage.h"



namespace
{
    using DirtySet = std::set<ve::ChunkPosition, ve::ChunkPositionLess>;



    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    bool samePosition( const ve::WorldBlockPosition& a, const ve::WorldBlockPosition& b ) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }

    bool nearlyEqual( float a, float b, float epsilon = 0.001f ) {
        return std::fabs(a - b) <= epsilon;
    }

    DirtySet consumeDirty( ve::VoxelStorage& world ) {
        DirtySet result;

        world.processDirtyChunks([&]( const ve::ChunkPosition& position ) {
            result.insert(position);
        });

        return result;
    }

    void expectDirty( ve::VoxelStorage& world, std::initializer_list<ve::ChunkPosition> expected ) {
        const DirtySet actual = consumeDirty(world);

        require(actual.size() == expected.size(), "Unexpected number of Dirty Chunks");

        for (const ve::ChunkPosition& position : expected) {
            require(actual.find(position) != actual.end(), "Expected Dirty Chunk is missing");
        }

        require(world.dirtyChunkCount() == 0, "Dirty queue was not consumed");
    }

    void expectBlock( const ve::VoxelStorage& world, const ve::WorldBlockPosition& position, ve::BlockId type ) {
        require(world.blockAt(position) == type, "Unexpected VoxelStorage BlockId");
    }

    void expectMesh( const ve::VoxelStorage& world, const ve::ChunkPosition& position, std::size_t indices ) {
        const ve::MeshData mesh = ve::ChunkMeshBuilder::create(world, position);

        require(mesh.indexCount() == indices, "Unexpected Chunk Mesh index count");
        require(mesh.triangleCount() == indices / 3, "Unexpected Chunk Mesh triangle count");
    }

    void expectHit( const ve::VoxelStorage& world, const ve::Camera& camera,
                    const ve::WorldBlockPosition& expected, float reach = 6.0f ) {
        const auto hit = ve::VoxelRaycaster::cast(world, camera.position(), camera.forward(), reach);

        require(hit.has_value(), "Camera ray did not hit the expected block");
        require(samePosition(hit->block, expected), "Camera ray selected the wrong block");
    }

    void expectBroken( ve::VoxelStorage& world, const ve::Camera& camera,
                       const ve::WorldBlockPosition& expected, float reach = 6.0f ) {
        const auto result = ve::BlockInteractor::breakBlock(
            world, camera.position(), camera.forward(), reach
        );

        require(result.has_value(), "Camera-driven block breaking failed");
        require(samePosition(result->block, expected), "Camera broke the wrong block");
        expectBlock(world, expected, ve::BlockId::Air);
    }

    void expectPlaced( ve::VoxelStorage& world, const ve::Camera& camera,
                       const ve::WorldBlockPosition& expected, ve::BlockId type,
                       float reach = 6.0f ) {
        const auto result = ve::BlockInteractor::placeBlock(
            world, camera.position(), camera.forward(), reach, type
        );

        require(result.has_value(), "Camera-driven block placement failed");
        require(samePosition(result->placedBlock, expected), "Camera placed the wrong block");
        expectBlock(world, expected, type);
    }



    void testForwardMovementChangesReach() {
        ve::VoxelStorage world{};
        const ve::WorldBlockPosition target{ 0, 1, 2 };
        const ve::ChunkPosition chunk{ 0, 0, 0 };

        world.setBlock(target, ve::BlockId::Stone);
        consumeDirty(world);

        ve::Camera camera{};
        camera.setPosition(ve::Vec3{ 0.5f, 1.5f, 5.5f });
        camera.lookAt(ve::Vec3{ 0.5f, 1.5f, 2.5f });

        require(!ve::VoxelRaycaster::cast(world, camera.position(), camera.forward(), 2.0f).has_value(),
                "Block should initially be out of reach");

        ve::CameraController controller{};
        controller.setMoveSpeed(2.0f);

        ve::CameraMovementInput input{};
        input.forward = true;
        controller.update(camera, input, 0.5f);

        require(nearlyEqual(camera.position().z, 4.5f), "Forward movement distance is incorrect");
        expectHit(world, camera, target, 2.0f);
        expectBroken(world, camera, target, 2.0f);
        expectDirty(world, { chunk });
        expectMesh(world, chunk, 0);

        std::cout << "[PASS] Camera movement changes reach and break target\n";
    }



    void testLatestLookDirectionWins() {
        ve::VoxelStorage world{};
        const ve::WorldBlockPosition alongX{ 2, 1, 0 };
        const ve::WorldBlockPosition alongNegativeZ{ 0, 1, -2 };

        world.setBlock(alongX, ve::BlockId::Stone);
        world.setBlock(alongNegativeZ, ve::BlockId::Grass);
        consumeDirty(world);

        ve::Camera camera{};
        camera.setPosition(ve::Vec3{ 0.5f, 1.5f, 0.5f });
        camera.lookAt(ve::Vec3{ 2.5f, 1.5f, 0.5f });

        expectHit(world, camera, alongX);

        // Simulate the Camera Turning Between Selection and Click.
        camera.lookAt(ve::Vec3{ 0.5f, 1.5f, -2.5f });

        expectHit(world, camera, alongNegativeZ);
        expectBroken(world, camera, alongNegativeZ);
        expectBlock(world, alongX, ve::BlockId::Stone);
        expectDirty(world, { ve::ChunkPosition{ 0, 0, -1 } });

        std::cout << "[PASS] Latest Camera direction determines the broken block\n";
    }



    void testVerticalPitchSelection() {
        ve::VoxelStorage world{};
        const ve::WorldBlockPosition above{ 0, 3, 0 };
        const ve::WorldBlockPosition below{ 0, -3, 0 };

        world.setBlock(above, ve::BlockId::Stone);
        world.setBlock(below, ve::BlockId::Dirt);

        ve::Camera camera{};
        camera.setPosition(ve::Vec3{ 0.5f, 0.5f, 0.5f });

        camera.setYawPitch(-90.0f, 200.0f);
        require(nearlyEqual(camera.pitchDegrees(), 89.0f), "Upward pitch was not clamped");
        expectHit(world, camera, above);

        camera.setYawPitch(-90.0f, -200.0f);
        require(nearlyEqual(camera.pitchDegrees(), -89.0f), "Downward pitch was not clamped");
        expectHit(world, camera, below);

        std::cout << "[PASS] Camera pitch clamp preserves vertical selection\n";
    }



    void testSelectionRefreshAfterBreakAndPlace() {
        ve::VoxelStorage world{};
        const ve::WorldBlockPosition front{ 0, 1, 1 };
        const ve::WorldBlockPosition back{ 0, 1, 0 };
        const ve::ChunkPosition chunk{ 0, 0, 0 };

        world.setBlock(front, ve::BlockId::Grass);
        world.setBlock(back, ve::BlockId::Stone);
        consumeDirty(world);

        ve::Camera camera{};
        camera.setPosition(ve::Vec3{ 0.5f, 1.5f, 5.5f });
        camera.lookAt(ve::Vec3{ 0.5f, 1.5f, 0.5f });

        expectHit(world, camera, front);
        expectMesh(world, chunk, 60);

        expectBroken(world, camera, front);
        expectHit(world, camera, back);
        expectMesh(world, chunk, 36);

        expectPlaced(world, camera, front, ve::BlockId::Dirt);
        expectHit(world, camera, front);
        expectBlock(world, back, ve::BlockId::Stone);
        expectMesh(world, chunk, 60);
        expectDirty(world, { chunk });

        std::cout << "[PASS] Ray selection refreshes immediately after break and place\n";
    }



    void testPositiveChunkBoundaryWithCamera() {
        ve::VoxelStorage world{};
        const ve::WorldBlockPosition nearBlock{ 15, 1, 1 };
        const ve::WorldBlockPosition farBlock{ 16, 1, 1 };
        const ve::ChunkPosition nearChunk{ 0, 0, 0 };
        const ve::ChunkPosition farChunk{ 1, 0, 0 };

        world.setBlock(nearBlock, ve::BlockId::Stone);
        world.setBlock(farBlock, ve::BlockId::Dirt);
        consumeDirty(world);

        ve::Camera camera{};
        camera.setPosition(ve::Vec3{ 13.5f, 1.5f, 1.5f });
        camera.lookAt(ve::Vec3{ 16.5f, 1.5f, 1.5f });

        expectHit(world, camera, nearBlock);
        expectMesh(world, nearChunk, 30);
        expectMesh(world, farChunk, 30);

        expectBroken(world, camera, nearBlock);
        expectHit(world, camera, farBlock);
        expectDirty(world, { nearChunk, farChunk });
        expectMesh(world, nearChunk, 0);
        expectMesh(world, farChunk, 36);

        expectPlaced(world, camera, nearBlock, ve::BlockId::Grass);
        expectDirty(world, { nearChunk, farChunk });
        expectMesh(world, nearChunk, 30);
        expectMesh(world, farChunk, 30);

        std::cout << "[PASS] Camera interaction across positive Chunk boundary\n";
    }



    void testNegativeChunkBoundaryWithCamera() {
        ve::VoxelStorage world{};
        const ve::WorldBlockPosition nearBlock{ -16, 1, 1 };
        const ve::WorldBlockPosition farBlock{ -17, 1, 1 };
        const ve::ChunkPosition nearChunk{ -1, 0, 0 };
        const ve::ChunkPosition farChunk{ -2, 0, 0 };

        world.setBlock(nearBlock, ve::BlockId::Grass);
        world.setBlock(farBlock, ve::BlockId::Dirt);
        consumeDirty(world);

        ve::Camera camera{};
        camera.setPosition(ve::Vec3{ -13.5f, 1.5f, 1.5f });
        camera.lookAt(ve::Vec3{ -17.5f, 1.5f, 1.5f });

        expectHit(world, camera, nearBlock);
        expectMesh(world, nearChunk, 30);
        expectMesh(world, farChunk, 30);

        expectBroken(world, camera, nearBlock);
        expectHit(world, camera, farBlock);
        expectDirty(world, { nearChunk, farChunk });
        expectMesh(world, nearChunk, 0);
        expectMesh(world, farChunk, 36);

        expectPlaced(world, camera, nearBlock, ve::BlockId::Stone);
        expectDirty(world, { nearChunk, farChunk });
        expectMesh(world, nearChunk, 30);
        expectMesh(world, farChunk, 30);

        std::cout << "[PASS] Camera interaction across negative Chunk boundary\n";
    }



    void testLookingAwayPreventsEdits() {
        ve::VoxelStorage world{};
        const ve::WorldBlockPosition block{ 0, 1, -2 };
        const ve::ChunkPosition chunk{ 0, 0, -1 };

        world.setBlock(block, ve::BlockId::Stone);
        consumeDirty(world);

        ve::Camera camera{};
        camera.setPosition(ve::Vec3{ 0.5f, 1.5f, 0.5f });
        camera.lookAt(ve::Vec3{ 0.5f, 1.5f, -2.5f });
        expectHit(world, camera, block);

        camera.lookAt(ve::Vec3{ 0.5f, 1.5f, 3.5f });

        const auto broken = ve::BlockInteractor::breakBlock(
            world, camera.position(), camera.forward(), 6.0f
        );
        const auto placed = ve::BlockInteractor::placeBlock(
            world, camera.position(), camera.forward(), 6.0f, ve::BlockId::Dirt
        );

        require(!broken.has_value() && !placed.has_value(), "Looking away changed the VoxelStorage");
        expectBlock(world, block, ve::BlockId::Stone);
        expectDirty(world, { });
        expectMesh(world, chunk, 36);
        require(world.chunkCount() == 1, "A missed interaction created a Chunk");

        std::cout << "[PASS] Looking away prevents VoxelStorage modifications\n";
    }



    void testCameraCellPlacementIsRejected() {
        ve::VoxelStorage world{};
        const ve::WorldBlockPosition anchor{ 1, 1, 0 };
        const ve::WorldBlockPosition cameraCell{ 0, 1, 0 };
        const ve::ChunkPosition chunk{ 0, 0, 0 };

        world.setBlock(anchor, ve::BlockId::Stone);
        consumeDirty(world);

        ve::Camera camera{};
        camera.setPosition(ve::Vec3{ 0.5f, 1.5f, 0.5f });
        camera.lookAt(ve::Vec3{ 1.5f, 1.5f, 0.5f });
        expectHit(world, camera, anchor);

        const auto placed = ve::BlockInteractor::placeBlock(
            world, camera.position(), camera.forward(), 6.0f, ve::BlockId::Dirt
        );

        require(!placed.has_value(), "Placement inside Camera cell must be rejected");
        expectBlock(world, cameraCell, ve::BlockId::Air);
        expectBlock(world, anchor, ve::BlockId::Stone);
        expectDirty(world, { });
        expectMesh(world, chunk, 36);

        std::cout << "[PASS] Camera cell remains empty after placement attempt\n";
    }



    void testStrafeThenTurnAndPlace() {
        ve::VoxelStorage world{};
        const ve::WorldBlockPosition anchor{ 3, 1, 0 };
        const ve::WorldBlockPosition placedBlock{ 2, 1, 0 };
        const ve::ChunkPosition chunk{ 0, 0, 0 };

        world.setBlock(anchor, ve::BlockId::Stone);
        consumeDirty(world);

        ve::Camera camera{};
        camera.setPosition(ve::Vec3{ 0.5f, 1.5f, 0.5f });
        camera.lookAt(ve::Vec3{ 0.5f, 1.5f, -3.5f });
        require(!ve::VoxelRaycaster::cast(world, camera.position(), camera.forward(), 6.0f).has_value(),
                "Camera should initially look away from the anchor");

        ve::CameraController controller{};
        controller.setMoveSpeed(2.0f);

        ve::CameraMovementInput input{};
        input.right = true;
        controller.update(camera, input, 0.5f);

        require(nearlyEqual(camera.position().x, 1.5f), "Right strafe movement is incorrect");

        camera.lookAt(ve::Vec3{ 3.5f, 1.5f, 0.5f });
        expectHit(world, camera, anchor);
        expectPlaced(world, camera, placedBlock, ve::BlockId::Grass);
        expectHit(world, camera, placedBlock);
        expectDirty(world, { chunk });
        expectMesh(world, chunk, 60);

        std::cout << "[PASS] Strafe, turn, place, and refresh selection\n";
    }
}



int main() {
    try {
        testForwardMovementChangesReach();
        testLatestLookDirectionWins();
        testVerticalPitchSelection();
        testSelectionRefreshAfterBreakAndPlace();
        testPositiveChunkBoundaryWithCamera();
        testNegativeChunkBoundaryWithCamera();
        testLookingAwayPreventsEdits();
        testCameraCellPlacementIsRejected();
        testStrafeThenTurnAndPlace();

        std::cout << "\nAll Camera Interaction Integration tests passed.\n";
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
