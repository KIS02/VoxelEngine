#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "gameplay/core/GameplaySession.h"


namespace
{
    void require( bool ok, const char* error ) {
        if (!ok)
            throw std::runtime_error(error);
    }

    void testPlayerAndModes() {
        ve::GameplaySession gameplay;
        gameplay.start(ve::Vec3{ 8.5f, 21.5f, 27.5f });
        require(gameplay.player().spawned, "Player did not spawn");
        require(gameplay.player().position.x == 8.5f, "Spawn position lost");
        require(gameplay.player().mode == ve::GameMode::Creative, "Default mode changed");
        require(gameplay.events().take()[0].type == ve::GameplayEventType::PlayerSpawned, "Missing spawn event");

        gameplay.synchronizeCreativePosition(ve::Vec3{ 11.0f, 10.0f, 9.0f });
        require(gameplay.player().position.x == 11.0f, "Creative Camera sync failed");
        gameplay.toggleMode();
        require(gameplay.player().mode == ve::GameMode::Survival, "Mode toggle failed");
        gameplay.synchronizeCreativePosition(ve::Vec3{ 100.0f, 0.0f, 0.0f });
        require(gameplay.player().position.x == 11.0f, "Survival must not inherit Creative free-flight movement");
        const auto events = gameplay.events().take();
        require(events.size() == 1 && events[0].type == ve::GameplayEventType::GameModeChanged, "Missing mode event");

        gameplay.toggleMode();
        require(gameplay.player().mode == ve::GameMode::Creative, "Returning to Creative failed");
        ve::PlayerSpawn::setSpawnPoint(gameplay.player(), ve::Vec3{ 3.0f, 4.0f, 5.0f });
        gameplay.respawn();
        require(gameplay.player().position.y == 4.0f && gameplay.player().velocity.lengthSquared() == 0.0f,
                "Respawn did not reset player state");

        bool rejected = false;
        try {
            gameplay.player().setPosition(ve::Vec3{ NAN, 0.0f, 0.0f });
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "NaN PlayerPosition accepted");
        std::cout << "[PASS] PlayerState, Spawn/Respawn, Creative/Survival and events\n";
    }

    void testFixedClock() {
        ve::FixedGameplayClock clock(0.01, 8);
        int executed = 0;
        for (int i = 0; i < 20; ++i) {
            clock.advance(0.005, [&]( float dt ) {
                require(std::fabs(dt - 0.01f) < 1.0e-6f, "Unexpected fixed dt");
                ++executed;
            });
        }
        require(executed == 10 && clock.ticks() == 10, "Variable frame rate changed fixed tick count");

        const auto slow = clock.advance(0.2, [&]( float ) { ++executed; });
        require(slow.steps == 8 && slow.discardedSeconds > 0.1, "Low-FPS catch-up policy not applied");
        require(slow.interpolationAlpha >= 0.0f && slow.interpolationAlpha < 1.0f,
                "Invalid interpolation alpha");
        require(clock.totalDiscardedSeconds() > 0.0, "Discarded simulation time not reported");

        bool rejected = false;
        try {
            clock.advance(-0.01, [&]( float ) {});
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "Negative frame delta accepted");
        std::cout << "[PASS] Fixed Tick determinism, catch-up cap, discarded time, interpolation\n";
    }

    void testCooldownsAndEvents() {
        ve::GameplaySession session;
        session.start(ve::Vec3{});
        session.events().take();
        session.cooldowns().start("attack", 0.05f);
        require(!session.cooldowns().ready("attack"), "Cooldown should be active");
        for (int i = 0; i < 4; ++i)
            session.advance(1.0f / 60.0f);
        require(session.cooldowns().ready("attack"), "Cooldown did not expire");
        const auto events = session.events().take();
        require(events.size() == 1 && events[0].type == ve::GameplayEventType::CooldownExpired &&
                events[0].subject == "attack", "Cooldown expiry event missing");
        std::cout << "[PASS] Cooldown expiry through fixed Gameplay Tick\n";
    }
}


int main() {
    try {
        testPlayerAndModes();
        testFixedClock();
        testCooldownsAndEvents();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
