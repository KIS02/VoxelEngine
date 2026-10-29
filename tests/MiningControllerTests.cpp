#include <iostream>
#include <stdexcept>

#include "gameplay/mining/MiningController.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}

int main() {
    try {
        ve::ItemRegistry items;
        ve::Inventory inventory(items);
        ve::WorldItems drops(items);
        ve::GameplayBlockCatalog blocks;
        ve::VoxelStorage world;
        const ve::WorldBlockPosition target{ 0, 1, 0 };
        const ve::Vec3 eye{ 0.5f, 1.5f, 3.5f };
        const ve::Vec3 forward{ 0.0f, 0.0f, -1.0f };
        world.setBlock(target, ve::BlockId::Stone);
        inventory.set(0, ve::ItemStack{ ve::ItemId::WoodenPickaxe, 1 });

        ve::MiningController mining;
        auto progress = mining.advance(0.1f, true, eye, forward, 6.0f, world, blocks, inventory, drops);
        require(progress.normalized > 0.0f && !progress.completed, "Mining progresses over time");
        mining.advance(0.1f, false, eye, forward, 6.0f, world, blocks, inventory, drops);
        require(!mining.progress().block, "Releasing input resets mining");
        progress = mining.advance(0.1f, true, eye, forward, 6.0f, world, blocks, inventory, drops);
        require(progress.normalized < 0.25f, "Progress restarts on new hold");

        bool completed = false;
        for (int i = 0; i < 40; ++i) {
            progress = mining.advance(1.0f / 60.0f, true, eye, forward, 6.0f, world, blocks, inventory, drops);
            if (progress.completed) { completed = true; break; }
        }
        require(completed, "Pickaxe breaks stone in finite time");
        require(world.blockAt(target) == ve::BlockId::Air, "Mining changes world");
        require(drops.all().size() == 1 && drops.all().front().stack.id == ve::ItemId::Stone,
                "Mining creates matching world drop");
        require(inventory.at(0)->metadata.at("durability") == "63", "Successful mining costs one durability");
        require(world.dirtyChunkCount() > 0, "Mining schedules mesh rebuild");
        std::cout << "[PASS] Timed mining, cancel, tool speed, drop, durability, Dirty Chunk\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
