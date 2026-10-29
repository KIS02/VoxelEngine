#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "gameplay/physics/CharacterController.h"
#include "gameplay/physics/PhysicsWorld.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    void testSweptAabb() {
        const ve::CollisionAabb body{ ve::Vec3{ 0.0f, 1.0f, 0.0f }, ve::Vec3{ 0.6f, 2.8f, 0.6f } };
        const ve::CollisionAabb wall{ ve::Vec3{ 2.0f, 0.0f, 0.0f }, ve::Vec3{ 3.0f, 3.0f, 1.0f } };
        const auto hit = ve::sweepAabb(body, wall, ve::Vec3{ 3.0f, 0.0f, 0.0f });
        require(hit.has_value() && std::fabs(hit->fraction - 1.4f / 3.0f) < 0.0001f,
                "Swept collision missed a wall or returned a wrong time");
        require(hit->normal.x == -1.0f, "Swept collision normal should oppose motion");
        require(!ve::sweepAabb(body, wall, ve::Vec3{ 0.0f, 0.0f, 3.0f }).has_value(),
                "Swept collision reported a false positive");
        require(!ve::sweepAabb(body, wall, ve::Vec3{ -3.0f, 0.0f, 0.0f }).has_value(),
                "Swept collision rejected motion away from the wall");
        std::cout << "[PASS] Continuous swept AABB fraction/normal/away tests\n";
    }

    void testVoxelQueriesAndPlacement() {
        ve::VoxelStorage storage;
        storage.setBlock(ve::WorldBlockPosition{ -1, 0, 0 }, ve::BlockId::Stone);
        storage.setBlock(ve::WorldBlockPosition{ 0, 0, 0 }, ve::BlockId::Dirt);
        ve::PhysicsWorld world(storage);
        require(world.intersects(ve::CollisionAabb{ ve::Vec3{ -0.8f, 0.2f, 0.1f },
            ve::Vec3{ -0.2f, 1.2f, 0.9f } }), "Negative coordinate voxel missed");
        require(!world.intersects(ve::CollisionAabb{ ve::Vec3{ 0.2f, 1.1f, 0.2f },
            ve::Vec3{ 0.8f, 2.0f, 0.8f } }), "Voxel collision rejected empty space");
        const ve::CollisionAabb player{ ve::Vec3{ 0.2f, 1.0f, 0.2f },
            ve::Vec3{ 0.8f, 2.8f, 0.8f } };
        require(!world.canPlaceBlock(ve::WorldBlockPosition{ 0, 1, 0 }, player),
                "Placing a block around player feet should be rejected");
        require(world.canPlaceBlock(ve::WorldBlockPosition{ 0, 3, 0 }, player),
                "Placing a non-overlapping block should be allowed");
        const auto spawn = world.findSafeEyePosition(ve::Vec3{ 0.5f, 4.0f, 0.5f }, 0.3f, 1.8f, 1.62f);
        require(spawn.has_value() && std::fabs(spawn->y - 2.62f) < 0.01f,
                "Safe eye spawn must be above the supporting block");
        std::cout << "[PASS] Voxel negative coordinates, placement, safe spawn\n";
    }

    void testDynamicColliderValidation() {
        ve::VoxelStorage storage;
        ve::PhysicsWorld world(storage);
        world.setDynamicColliders({ ve::DynamicCollider{ 1,
            ve::CollisionAabb{ ve::Vec3{ 1.0f, 0.0f, 0.0f }, ve::Vec3{ 2.0f, 1.0f, 1.0f } },
            ve::Vec3{ 0.0f, 0.0f, 0.0f }, true } });
        require(world.intersects(ve::CollisionAabb{ ve::Vec3{ 1.1f, 0.1f, 0.1f },
            ve::Vec3{ 1.5f, 0.9f, 0.9f } }), "Dynamic collider not queried");
        bool rejected = false;
        try {
            world.setDynamicColliders({ world.dynamicColliders().front(), world.dynamicColliders().front() });
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && world.dynamicColliders().size() == 1,
                "Duplicate dynamic IDs should be rejected without changing the existing set");
        std::cout << "[PASS] Dynamic collider query and strong exception guarantee\n";
    }
}


int main() {
    try {
        testSweptAabb();
        testVoxelQueriesAndPlacement();
        testDynamicColliderValidation();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
