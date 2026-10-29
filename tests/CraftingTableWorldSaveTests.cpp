#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "voxel/save/WorldSaveFile.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}

int main() {
    const auto path = std::filesystem::temp_directory_path() / "ve_phase12_4_table_world.vew";
    try {
        ve::WorldSaveData source{};
        source.deltas[ve::ChunkPosition{ 0, 0, 0 }].blocks.push_back(
            ve::BlockDelta{ 17, ve::BlockId::CraftingTable });
        ve::writeWorldSaveFile(path, source);
        const auto restored = ve::readWorldSaveFile(path);
        const auto& block = restored.deltas.at(ve::ChunkPosition{ 0, 0, 0 }).blocks.at(0);
        require(block.index == 17 && block.type == ve::BlockId::CraftingTable,
                "New crafting table BlockId must round-trip through existing WorldSave format");
        std::filesystem::remove(path);
        std::cout << "[PASS] Crafting table persistence retains WorldSave V1 format and stable IDs\n";
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove(path);
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
