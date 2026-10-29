#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "gameplay/crafting/CraftingGrid.h"
#include "gameplay/tools/ToolRules.h"


namespace
{
    void require( bool valid, const char* message ) {
        if (!valid)
            throw std::runtime_error(message);
    }
}

int main() {
    try {
        ve::ItemRegistry items;
        ve::Inventory inventory(items);
        ve::CraftingGrid grid(items);
        require(grid.recipes().size() == 6, "Fallback recipe set");
        for (int i = 0; i < 4; ++i)
            grid.slots().set(i, ve::ItemStack{ ve::ItemId::Stone, 1 });
        require(grid.preview()->id == ve::ItemId::CraftingTable, "2x2 crafting table recipe");
        require(grid.craft(inventory), "2x2 crafting succeeds");
        require(inventory.at(0)->id == ve::ItemId::CraftingTable, "Table must enter inventory");
        for (int i = 0; i < 4; ++i)
            require(!grid.slots().at(i), "Crafting must consume one item per occupied cell");

        for (std::size_t i = 0; i < inventory.size(); ++i)
            inventory.set(i, ve::ItemStack{ ve::ItemId::Dirt, 64 });
        for (int i = 0; i < 4; ++i)
            grid.slots().set(i, ve::ItemStack{ ve::ItemId::Stone, 1 });
        require(!grid.craft(inventory), "Full inventory rejects output");
        for (int i = 0; i < 4; ++i)
            require(grid.slots().at(i)->count == 1, "Failed craft must preserve inputs");

        ve::WorldItems drops(items);
        grid.putAway(inventory, drops, ve::Vec3{ 1.0f, 2.0f, 3.0f });
        require(drops.all().size() == 4, "No free inventory: return inputs to world");
        inventory.set(0, std::nullopt);
        grid.setWorkbench(true);
        const ve::ItemId pickaxe[9]{ ve::ItemId::Grass, ve::ItemId::Grass, ve::ItemId::Grass,
            ve::ItemId::None, ve::ItemId::Dirt, ve::ItemId::None,
            ve::ItemId::None, ve::ItemId::Dirt, ve::ItemId::None };
        for (int i = 0; i < 9; ++i) {
            if (pickaxe[i] != ve::ItemId::None)
                grid.slots().set(i, ve::ItemStack{ pickaxe[i], 1 });
        }
        require(grid.preview()->id == ve::ItemId::WoodenPickaxe, "Shaped 3x3 tool recipe");
        require(grid.craft(inventory), "3x3 recipe result");
        require(inventory.at(0)->id == ve::ItemId::WoodenPickaxe, "Crafted pickaxe inserted");
        require(!ve::ToolRules::consumeUse(inventory, 0), "First use should not break tool");
        require(inventory.at(0)->metadata.at("durability") == "63", "Durability is saved in ItemStack metadata");
        for (int i = 0; i < 63; ++i)
            ve::ToolRules::consumeUse(inventory, 0);
        require(!inventory.at(0), "Tool breaks on last successful use");

        const std::filesystem::path path = std::filesystem::temp_directory_path() / "ve_12_4_invalid_recipe.txt";
        {
            std::ofstream out(path);
            out << "2|2,2,2,2|103|1\n2|2,2,2,2|103|1\n";
        }
        bool rejected = false;
        const std::size_t previous = grid.recipes().size();
        try { grid.recipes().loadFile(path, items); }
        catch (const std::exception&) { rejected = true; }
        std::filesystem::remove(path);
        require(rejected && grid.recipes().size() == previous, "Invalid recipe file rejected atomically");
        std::cout << "[PASS] 2x2 / 3x3, output rollback, durability, overflow fallback, data validation\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
