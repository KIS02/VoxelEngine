#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "gameplay/save/PlayerSave.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        const auto file = std::filesystem::temp_directory_path() / "phase12_5_player_test.vep";
        std::filesystem::remove(file);
        ve::PlayerState initial;
        initial.initialize(ve::Vec3{ -3.5f, 12.5f, 9.5f });
        initial.setMode(ve::GameMode::Survival);
        initial.position = ve::Vec3{ 12.0f, 5.25f, -8.0f };
        initial.health = 9.0f;
        initial.hunger = 7.5f;
        initial.stamina = 45.0f;
        require(!ve::PlayerSave::read(file, initial), "Missing save must return false");
        ve::PlayerSave::write(file, initial);
        ve::PlayerState restored;
        restored.initialize(ve::Vec3{});
        require(ve::PlayerSave::read(file, restored), "Save not loaded");
        require(restored.mode == ve::GameMode::Survival && restored.position.z == -8.0f &&
                restored.spawnPosition.x == -3.5f && restored.health == 9.0f &&
                restored.hunger == 7.5f && restored.stamina == 45.0f,
                "Save must preserve player position, mode, needs and spawn");

        { std::fstream data(file, std::ios::in | std::ios::out | std::ios::binary);
          data.seekp(22); data.put(static_cast<char>(0xFF)); }
        const ve::PlayerState snapshot = restored;
        bool rejected = false;
        try { (void)ve::PlayerSave::read(file, restored); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Checksum corruption must be rejected");
        require(restored.health == snapshot.health && restored.position.z == snapshot.position.z,
                "Invalid save must not change the destination");
        std::filesystem::remove(file);
        ve::PlayerSave::write(file, initial);
        {
            std::ofstream data(file, std::ios::binary | std::ios::app);
            data.put(static_cast<char>(0xAA));
        }
        rejected = false;
        try {
            (void)ve::PlayerSave::read(file, restored);
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        require(rejected, "Appended bytes must be rejected");
        require(restored.health == snapshot.health, "Oversized save must not mutate destination");

        std::filesystem::resize_file(file, 12);
        rejected = false;
        try {
            (void)ve::PlayerSave::read(file, restored);
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        require(rejected, "Truncated player save must be rejected");
        require(restored.health == snapshot.health, "Truncated save must not mutate destination");
        std::filesystem::remove(file);

        ve::PlayerState invalid = initial;
        invalid.health = -1.0f;
        rejected = false;
        try {
            ve::PlayerSave::write(file, invalid);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && !std::filesystem::exists(file), "Invalid state must not overwrite save");
        std::cout << "[PASS] Player save V1, round-trip, checksum, bounds and rollback\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
