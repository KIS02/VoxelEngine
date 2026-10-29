#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "gameplay/items/InventorySave.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}


int main() {
    const auto path = std::filesystem::temp_directory_path() / "voxel_phase12_3_inventory_test.vei";
    try {
        ve::ItemRegistry items;
        ve::Inventory source(items);
        source.selectHotbar(4);
        source.set(0, ve::ItemStack{ ve::ItemId::Dirt, 23, { { "owner", "test" }, { "grade", "1" } } });
        source.set(22, ve::ItemStack{ ve::ItemId::WoodenPickaxe, 1 });
        ve::WorldItems world(items);
        world.spawn(ve::ItemStack{ ve::ItemId::Grass, 4 }, ve::Vec3{ -2.5f, 12.0f, 1.5f });
        ve::InventorySave::write(path, source, &world);
        ve::Inventory restored(items);
        ve::WorldItems worldRestored(items);
        require(ve::InventorySave::read(path, restored, &worldRestored), "File should exist");
        require(worldRestored.all().size() == 1 && worldRestored.all()[0].stack.count == 4 &&
                worldRestored.all()[0].position.x == -2.5f, "World drops round-trip");
        require(restored.selectedHotbar() == 4, "Selected slot persisted");
        require(restored.at(0)->count == 23 && restored.at(0)->metadata.at("owner") == "test",
                "Stack metadata must round-trip");
        require(restored.at(22)->id == ve::ItemId::WoodenPickaxe, "Tool round-trip");

        std::fstream corrupt(path, std::ios::in | std::ios::out | std::ios::binary);
        corrupt.seekp(6);
        corrupt.put(static_cast<char>(255));
        corrupt.close();
        bool rejected = false;
        try {
            (void)ve::InventorySave::read(path, restored, &worldRestored);
        } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Corrupted save must be rejected");
        require(restored.at(0)->count == 23 && worldRestored.all().size() == 1,
                "Failed load must preserve inventory and world drops");
        std::filesystem::remove(path);
        std::cout << "[PASS] InventorySave: round-trip, metadata, checksum, atomic parse\n";
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove(path);
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
