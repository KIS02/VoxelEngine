#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "gameplay/block/OrientedBlocks.h"
#include "assets/block/BlockRegistry.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}

int main() {
    try {
        ve::OrientedBlocks blocks;
        const ve::WorldBlockPosition position{ -31, 2, 41 };
        const std::filesystem::path path = std::filesystem::temp_directory_path() / "ve_12_4_orientations.vbo";
        blocks.set(position, ve::OrientedBlocks::facing(ve::Vec3{ 1.0f, 0.0f, 0.0f }));
        require(blocks.get(position) == 1, "Facing derives from placement direction");
        blocks.save(path);
        ve::OrientedBlocks loaded;
        require(loaded.load(path) && loaded.get(position) == 1, "Orientation persists across restart");
        {
            std::ofstream corrupt(path, std::ios::trunc);
            corrupt << "invalid header\n";
        }
        bool rejected = false;
        try { loaded.load(path); }
        catch (const std::exception&) { rejected = true; }
        require(rejected && loaded.get(position) == 1, "Malformed save does not destroy current state");
        std::filesystem::remove(path);
        loaded.erase(position);
        require(loaded.size() == 0, "Destroyed block clears orientation");
        require(ve::BlockRegistry::definition(ve::BlockId::CraftingTable).solid,
                "New crafting table is solid and does not alter older IDs");
        std::cout << "[PASS] Facing, orientation persistence, bad-data rollback, block visual contract\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
