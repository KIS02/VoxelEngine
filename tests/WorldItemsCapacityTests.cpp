#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "gameplay/items/WorldItems.h"


namespace
{
    void require( bool valid, const char* message ) {
        if (!valid)
            throw std::runtime_error(message);
    }
}

int main() {
    try {
        ve::ItemRegistry registry;
        ve::WorldItems world(registry);
        const ve::Vec3 position{ 0.0f, 5.0f, 0.0f };
        std::uint64_t first = 0;

        for (int i = 0; i < 4096; ++i) {
            const std::uint64_t id = world.spawn(ve::ItemStack{ ve::ItemId::Dirt, 1 }, position);
            if (i == 0)
                first = id;
        }
        require(world.all().size() == 4096, "All 4096 world items should be accepted");

        bool rejected = false;
        try {
            world.spawn(ve::ItemStack{ ve::ItemId::Dirt, 1 }, position);
        } catch (const std::length_error&) {
            rejected = true;
        }
        require(rejected, "A 4097th item must be rejected before changing world state");
        require(world.all().size() == 4096, "Rejected spawn must preserve world items");

        require(world.remove(first), "Existing item can be removed");
        const std::uint64_t next = world.spawn(ve::ItemStack{ ve::ItemId::Stone, 1 }, position);
        require(next > first, "World item IDs remain monotonic after removal");
        require(world.all().size() == 4096, "Released capacity can be reused");
        world.restore(world.all());
        require(world.all().size() == 4096, "Maximum capacity remains restorable");

        std::cout << "[PASS] World item capacity, rejection atomicity, and save compatibility\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
