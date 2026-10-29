#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "gameplay/core/GameplaySession.h"
#include "gameplay/save/PlayerSave.h"


namespace
{
    void require( bool condition, const char* reason ) {
        if (!condition)
            throw std::runtime_error(reason);
    }
}


int main() {
    try {
        ve::VoxelStorage world;
        for (int x = -3; x <= 3; ++x) {
            for (int z = -3; z <= 3; ++z)
                world.setBlock(ve::WorldBlockPosition{ x, 0, z }, ve::BlockId::Stone);
        }
        ve::GameplaySession session;
        session.start(ve::Vec3{ 0.5f, 5.0f, 0.5f });
        session.configurePhysics(world);
        session.setMode(ve::GameMode::Survival);
        session.player().hunger = 8.0f;
        session.player().health = 12.0f;
        session.player().stamina = 30.0f;
        session.inventory().set(0, ve::ItemStack{ ve::ItemId::Bread, 1 });
        require(session.consumeSelectedFood(), "Session must consume equipped food");
        require(std::fabs(session.player().health - 14.0f) < 0.01f,
                "Session food use did not heal");

        const auto path = std::filesystem::temp_directory_path() / "phase12_5_session_test.vep";
        ve::PlayerSave::write(path, session.player());
        ve::GameplaySession second;
        second.start(ve::Vec3{ 0.5f, 4.0f, 0.5f });
        second.configurePhysics(world);
        ve::PlayerState loaded = second.player();
        require(ve::PlayerSave::read(path, loaded), "Saved session not loaded");
        second.restorePlayer(loaded);
        require(second.player().mode == ve::GameMode::Survival &&
                second.player().health == session.player().health &&
                second.player().hunger == session.player().hunger,
                "Gameplay session restore mismatch");
        second.advance(1.0f / 60.0f);
        require(second.player().alive, "Restored player should remain alive");
        second.player().health = 0.0f;
        second.player().alive = false;
        second.respawn();
        require(second.player().health == second.player().stats.maxHealth &&
                second.player().hunger == second.player().stats.maxHunger,
                "Respawn must restore health and hunger");
        std::filesystem::remove(path);
        std::cout << "[PASS] Food usage, fixed tick, player persistence and safe respawn integration\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
