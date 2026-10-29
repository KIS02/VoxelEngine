#include <cmath>
#include <iostream>
#include <stdexcept>

#include "gameplay/physics/CharacterController.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    void makeFloor( ve::VoxelStorage& storage ) {
        for (int x = -8; x <= 12; ++x) {
            for (int z = -5; z <= 5; ++z)
                storage.setBlock(ve::WorldBlockPosition{ x, 0, z }, ve::BlockId::Stone);
        }
    }

    ve::PlayerState createPlayer( const ve::Vec3& position ) {
        ve::PlayerState player;
        player.initialize(position);
        player.setMode(ve::GameMode::Survival);
        return player;
    }

    void testWalkGroundJumpAndLanding() {
        ve::VoxelStorage storage;
        makeFloor(storage);
        ve::PhysicsWorld world(storage);
        ve::CharacterController controller;
        ve::PlayerState player = createPlayer(ve::Vec3{ 0.5f, 2.621f, 0.5f });
        ve::PlayerMovementCommand command;
        command.viewForward = ve::Vec3{ 1.0f, 0.5f, 0.0f };
        command.forward = 1.0f;
        bool moved = false;
        for (int i = 0; i < 90; ++i) {
            controller.step(player, command,  world, 1.0f / 60.0f);
            moved = moved || player.position.x > 1.0f;
        }
        require(moved && player.grounded && std::fabs(player.position.y - 2.62f) < 0.01f,
                "Walking should preserve floor contact and follow camera heading");

        command.forward = 0.0f;
        command.jump = true;
        const auto jumped = controller.step(player, command, world, 1.0f / 60.0f);
        require(jumped.jumped && !player.grounded && player.velocity.y > 0.0f,
                "Ground jump edge was not applied");
        float highest = player.position.y;
        bool landed = false;
        for (int i = 0; i < 180; ++i) {
            const auto result = controller.step(player, command, world, 1.0f / 60.0f);
            require(!result.jumped, "Held jump must not retrigger");
            highest = std::max(highest, player.position.y);
            if (result.landed) {
                landed = true;
                break;
            }
        }
        require(highest > 3.0f && landed && player.grounded,
                "Jump apex and safe landing were not produced");
        std::cout << "[PASS] Floor walking, normalized camera heading, jump edge, landing\n";
    }

    void testWallSlidingAndSprint() {
        ve::VoxelStorage storage;
        makeFloor(storage);
        for (int y = 1; y <= 3; ++y) {
            for (int z = -2; z <= 2; ++z)
                storage.setBlock(ve::WorldBlockPosition{ 3, y, z }, ve::BlockId::Stone);
        }
        ve::PhysicsWorld world(storage);
        ve::CharacterController controller;
        ve::PlayerState player = createPlayer(ve::Vec3{ 1.5f, 2.621f, 0.5f });
        ve::PlayerMovementCommand command;
        command.viewForward = ve::Vec3{ 1.0f, 0.0f, 0.0f };
        command.forward = 1.0f;
        command.strafe = 1.0f;
        for (int i = 0; i < 35; ++i)
            controller.step(player, command, world, 1.0f / 60.0f);
        require(player.position.x < 2.71f && player.position.z > 0.6f,
                "Diagonal wall slide must block X without blocking Z");

        ve::CharacterController normal;
        ve::CharacterController sprint;
        ve::PlayerState walking = createPlayer(ve::Vec3{ -6.0f, 2.621f, -3.5f });
        ve::PlayerState running = walking;
        ve::PlayerMovementCommand forward;
        forward.viewForward = ve::Vec3{ 1.0f, 0.0f, 0.0f };
        forward.forward = 1.0f;
        for (int i = 0; i < 30; ++i) {
            normal.step(walking, forward, world, 1.0f / 60.0f);
            forward.sprint = true;
            sprint.step(running, forward, world, 1.0f / 60.0f);
            forward.sprint = false;
        }
        require(running.position.x > walking.position.x + 0.3f,
                "Sprint must exceed normal walk speed");
        std::cout << "[PASS] Sprint speed and diagonal wall slide\n";
    }

    void testFastMovementCannotTunnel() {
        ve::VoxelStorage storage;
        makeFloor(storage);
        for (int y = 1; y <= 3; ++y)
            storage.setBlock(ve::WorldBlockPosition{ 2, y, 0 }, ve::BlockId::Stone);
        ve::PhysicsWorld world(storage);
        ve::CharacterController controller;
        ve::PlayerState player = createPlayer(ve::Vec3{ 0.5f, 2.621f, 0.5f });
        player.velocity = ve::Vec3{ 200.0f, 0.0f, 0.0f };
        controller.step(player, ve::PlayerMovementCommand{}, world, 1.0f / 60.0f);
        require(player.position.x <= 1.701f,
                "High-speed Player movement tunneled through a full-height voxel wall");
        std::cout << "[PASS] High-speed swept collision prevents voxel tunneling\n";
    }

    void testStepUpAndCeiling() {
        ve::VoxelStorage storage;
        makeFloor(storage);
        ve::PhysicsWorld world(storage);
        world.setDynamicColliders({ ve::DynamicCollider{ 3,
            ve::CollisionAabb{ ve::Vec3{ 1.0f, 1.0f, -0.5f }, ve::Vec3{ 2.0f, 1.45f, 0.5f } },
            ve::Vec3{}, true } });
        ve::CharacterController controller;
        ve::PlayerState player = createPlayer(ve::Vec3{ 0.5f, 2.621f, 0.0f });
        ve::PlayerMovementCommand input{};
        input.viewForward = ve::Vec3{ 1.0f, 0.0f, 0.0f };
        input.forward = 1.0f;
        float maxFoot = 1.0f;
        for (int i = 0; i < 36; ++i) {
            controller.step(player, input, world, 1.0f / 60.0f);
            maxFoot = std::max(maxFoot, player.position.y - controller.collider().eyeHeight);
        }
        require(player.position.x > 1.3f && maxFoot > 1.40f,
                "Step Height must move player over a low obstacle without jumping");

        ve::VoxelStorage ceilingStorage;
        makeFloor(ceilingStorage);
        for (int x = -2; x <= 2; ++x) {
            for (int z = -2; z <= 2; ++z)
                ceilingStorage.setBlock(ve::WorldBlockPosition{ x, 3, z }, ve::BlockId::Stone);
        }
        ve::PhysicsWorld ceilingWorld(ceilingStorage);
        ve::CharacterController jumpController;
        ve::PlayerState jumper = createPlayer(ve::Vec3{ 0.5f, 2.621f, 0.5f });
        ve::PlayerMovementCommand jumpInput{};
        jumpInput.jump = true;
        bool hitCeiling = false;
        for (int i = 0; i < 45; ++i) {
            const auto frame = jumpController.step(jumper, jumpInput, ceilingWorld, 1.0f / 60.0f);
            hitCeiling = hitCeiling || frame.hitCeiling;
        }
        require(hitCeiling && jumper.position.y < 3.9f,
                "Jump must not pass through a low ceiling");
        std::cout << "[PASS] Low step climbing and overhead ceiling collision\n";
    }

    void testFallAndMovingBody() {
        ve::VoxelStorage storage;
        makeFloor(storage);
        ve::PhysicsWorld world(storage);
        ve::CharacterController controller;
        ve::PlayerState player = createPlayer(ve::Vec3{ 0.5f, 25.0f, 0.5f });
        bool damaged = false;
        for (int i = 0; i < 240; ++i) {
            const auto result = controller.step(player, ve::PlayerMovementCommand{}, world, 1.0f / 60.0f);
            damaged = damaged || result.pendingFallDamage > 0.0f;
            if (result.landed)
                break;
        }
        require(damaged && player.grounded, "A long fall must land and produce fall damage");

        ve::VoxelStorage noFloor;
        ve::PhysicsWorld movingWorld(noFloor);
        movingWorld.setDynamicColliders({ ve::DynamicCollider{ 7,
            ve::CollisionAabb{ ve::Vec3{ 0.4f, 0.0f, -1.0f }, ve::Vec3{ 0.7f, 3.0f, 1.0f } },
            ve::Vec3{ -20.0f, 0.0f, 0.0f }, true } });
        ve::PlayerState target = createPlayer(ve::Vec3{ 0.0f, 1.62f, 0.0f });
        ve::CharacterController other;
        other.step(target, ve::PlayerMovementCommand{}, movingWorld, 1.0f / 60.0f);
        require(target.position.x < -0.005f, "Moving collider should push an initially stationary player");
        std::cout << "[PASS] Fall damage threshold and moving dynamic obstacle\n";
    }
}


int main() {
    try {
        testWalkGroundJumpAndLanding();
        testWallSlidingAndSprint();
        testFastMovementCannotTunnel();
        testStepUpAndCeiling();
        testFallAndMovingBody();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
