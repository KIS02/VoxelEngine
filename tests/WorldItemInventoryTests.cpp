#include <iostream>
#include <stdexcept>

#include "gameplay/items/WorldItems.h"


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
        ve::Inventory inventory(items, 2, 1);
        ve::WorldItems world(items);
        world.spawn(ve::ItemStack{ ve::ItemId::Dirt, 17 }, ve::Vec3{ 0.5f, 1.0f, 0.0f });
        world.advance(0.4f, ve::Vec3{}, inventory, true);
        require(world.all().size() == 1 && !inventory.at(0), "Pickup cooldown");
        world.advance(0.3f, ve::Vec3{}, inventory, true);
        require(world.all().empty() && inventory.at(0)->count == 17, "World item collected");
        inventory.set(1, ve::ItemStack{ ve::ItemId::Stone, 64 });
        world.spawn(ve::ItemStack{ ve::ItemId::Stone, 5 }, ve::Vec3{ 0.5f, 1.0f, 0.0f });
        world.advance(1.0f, ve::Vec3{}, inventory, true);
        require(world.all().size() == 1 && world.all()[0].stack.count == 5,
                "Full inventory must not destroy dropped items");
        inventory.set(1, std::nullopt);
        world.advance(0.1f, ve::Vec3{}, inventory, true);
        require(world.all().empty() && inventory.at(1)->count == 5, "Delayed pickup after freeing slot");
        std::cout << "[PASS] World item: delay, distance, pickup, partial/full capacity\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
