#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

#include "gameplay/core/GameplaySession.h"
#include "gameplay/interaction/BlockInteractor.h"
#include "input/ApplicationInput.h"


namespace
{
    void require( bool ok, const char* message ) {
        if (!ok)
            throw std::runtime_error(message);
    }

    void makeFloor( ve::VoxelStorage& voxels ) {
        for (int x = -4; x <= 4; ++x) {
            for (int z = -4; z <= 4; ++z)
                voxels.setBlock(ve::WorldBlockPosition{ x, 0, z }, ve::BlockId::Stone);
        }
    }

    void testSessionAndInputs() {
        ve::VoxelStorage voxels;
        makeFloor(voxels);
        ve::GameplaySession session;
        session.start(ve::Vec3{ 0.5f, 8.0f, 0.5f });
        session.configurePhysics(voxels);
        session.events().take();
        session.toggleMode();
        require(session.player().mode == ve::GameMode::Survival &&
                std::fabs(session.player().position.y - 2.62f) < 0.01f,
                "Survival mode should find safe supported eye position");

        ve::PlayerMovementCommand movement;
        movement.viewForward = ve::Vec3{ 1.0f, 0.0f, 0.0f };
        movement.forward = 1.0f;
        session.setMovementCommand(movement);
        session.advance(0.1f);
        require(session.player().position.x > 0.5f && session.player().grounded,
                "Fixed Gameplay Tick did not move the survival player");
        bool movementEvent = false;
        for (const auto& event : session.events().take())
            movementEvent = movementEvent || event.type == ve::GameplayEventType::PlayerMoved;
        require(movementEvent, "No PlayerMoved event after a Physics tick");

        session.toggleMode();
        const float before = session.player().position.x;
        session.synchronizeCreativePosition(ve::Vec3{ before + 3.0f, 5.0f, 0.5f });
        session.advance(1.0f / 60.0f);
        require(std::fabs(session.player().position.x - before - 3.0f) < 0.001f,
                "Creative free-flight synchronization changed");

        ve::ApplicationInput input;
        ve::InputSnapshot snapshot;
        snapshot.respawn = true;
        require(input.process(snapshot, true).respawn, "Respawn key first press missing");
        require(!input.process(snapshot, true).respawn, "Held respawn key repeated");
        snapshot.respawn = false;
        input.process(snapshot, true);
        snapshot.respawn = true;
        require(input.process(snapshot, true).respawn, "Respawn key did not re-arm");
        std::cout << "[PASS] Fixed Gameplay/physics, mode restoration, G/R input edge isolation\n";
    }

    void testFallDamageAppliedAndRespawn() {
        ve::VoxelStorage voxels;
        makeFloor(voxels);
        ve::GameplaySession session;
        session.start(ve::Vec3{ 0.5f, 2.621f, 0.5f });
        session.configurePhysics(voxels);
        session.setMode(ve::GameMode::Survival);
        session.events().take();
        session.player().setPosition(ve::Vec3{ 0.5f, 24.0f, 0.5f });
        session.player().velocity = ve::Vec3{};
        session.player().grounded = false;
        for (int i = 0; i < 240 && session.player().alive; ++i)
            session.advance(1.0f / 60.0f);
        require(session.totalFallDamage() > 0.0f && session.player().health < session.player().stats.maxHealth,
                "Long fall must apply actual health damage and accumulate the event value");
        bool sawDamage = false;
        for (const auto& event : session.events().take())
            sawDamage = sawDamage || event.type == ve::GameplayEventType::PlayerFallDamaged;
        require(sawDamage, "Fall damage must publish a typed Gameplay event");
        session.respawn();
        require(session.player().alive && session.player().health == session.player().stats.maxHealth,
                "Respawn must restore health and collision-safe player position");
        std::cout << "[PASS] Actual fall HP damage, event and safe respawn\n";
    }

    void testPlacementPreventsBodyOverlap() {
        ve::VoxelStorage voxels;
        voxels.setBlock(ve::WorldBlockPosition{ 0, 0, 0 }, ve::BlockId::Stone);
        const ve::Vec3 eye{ 0.5f, 2.621f, 0.5f };
        const ve::Vec3 down{ 0.0f, -1.0f, 0.0f };
        const ve::CollisionAabb body{ ve::Vec3{ 0.2f, 1.001f, 0.2f },
            ve::Vec3{ 0.8f, 2.801f, 0.8f } };
        const auto prevented = ve::BlockInteractor::placeBlock(voxels,
            eye, down, 6.0f, ve::BlockId::Dirt, &body);
        require(!prevented.has_value(), "Block placement inside body must be rejected");
        require(voxels.blockAt(ve::WorldBlockPosition{ 0, 1, 0 }) == ve::BlockId::Air,
                "Rejected block placement mutated VoxelStorage");

        // The optional argument preserves the original legacy API and behavior.
        const auto legacy = ve::BlockInteractor::placeBlock(voxels,
            eye, down, 6.0f, ve::BlockId::Dirt);
        require(legacy.has_value() &&
                voxels.blockAt(ve::WorldBlockPosition{ 0, 1, 0 }) == ve::BlockId::Dirt,
                "Original BlockInteractor call contract was unexpectedly changed");
        std::cout << "[PASS] Body overlap rejects placement and preserves old API without world mutation\n";
    }
}


int main() {
    try {
        testSessionAndInputs();
        testPlacementPreventsBodyOverlap();
        testFallDamageAppliedAndRespawn();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
