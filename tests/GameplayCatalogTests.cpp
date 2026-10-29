#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "gameplay/core/GameplaySession.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    void writeText( const std::filesystem::path& path, const char* content ) {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file << content;
        if (!file)
            throw std::runtime_error("Cannot write test fixture");
    }
}


int main() {
    try {
        ve::GameplaySession session;
        require(session.items().size() == 8, "Missing builtin item definitions");
        require(session.blocks().size() == 5, "Missing builtin block definitions");
        require(session.blocks().definition(ve::BlockId::Stone).visual.solid, "Stone collision flag changed");
        require(session.items().definition(ve::ItemId::Stone).maxStack == 64, "Unexpected stack limit");
        session.loadCatalogs("missing_gameplay_blocks__12_1.txt", "missing_gameplay_items__12_1.txt");
        require(session.blocks().definition(ve::BlockId::Stone).hardness == 1.5f, "Missing files must use safe defaults");

        const auto base = std::filesystem::temp_directory_path() / "phase12_1_gameplay_catalog_test";
        std::filesystem::create_directories(base);
        const auto blocks = base / "blocks.txt";
        const auto items = base / "items.txt";
        writeText(blocks, "# id|name|hardness|drop\r\n3|Reinforced Stone|4.0|3\r\n");
        writeText(items, "# id|name|kind|maxStack\n3|Granite|block|32\n");
        session.loadCatalogs(blocks, items);
        require(session.blocks().definition(ve::BlockId::Stone).hardness == 4.0f, "Block hardness not loaded");
        require(session.items().definition(ve::ItemId::Stone).maxStack == 32, "Item max stack not loaded");
        require(session.blocks().definition(ve::BlockId::Stone).visual.materials.forFace(ve::BlockFace::Top) ==
                ve::BlockMaterialKey::Stone, "External config must not change voxel face material");

        writeText(items, "3|Duplicate A|block|32\n3|Duplicate B|block|32\n");
        bool rejected = false;
        try {
            session.loadCatalogs(blocks, items);
        } catch (const std::exception&) {
            rejected = true;
        }
        require(rejected, "Duplicate ItemId accepted");
        require(session.items().definition(ve::ItemId::Stone).name == "Granite", "Failed load partially committed");

        writeText(items, "3|Granite|block|32\n");
        writeText(blocks, "3|Stone|nan|3\n");
        rejected = false;
        try {
            session.loadCatalogs(blocks, items);
        } catch (const std::exception&) {
            rejected = true;
        }
        require(rejected, "Invalid hardness accepted");
        require(session.blocks().definition(ve::BlockId::Stone).hardness == 4.0f, "Failed block load partially committed");

        std::filesystem::remove_all(base);
        std::cout << "[PASS] Item/Block external files, stable IDs, atomic rollback, face material preservation\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
