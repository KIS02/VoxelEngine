#include <iostream>
#include <stdexcept>

#include "gameplay/items/Inventory.h"
#include "gameplay/items/InventoryCursor.h"


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
        ve::Inventory inv(items);
        require(inv.size() == 36 && inv.hotbarSize() == 9, "Inventory dimensions");
        require(inv.insert(ve::ItemStack{ ve::ItemId::Dirt, 40 }) == 0, "Initial insert");
        require(inv.insert(ve::ItemStack{ ve::ItemId::Dirt, 30 }) == 0, "Merge then extra slot");
        require(inv.at(0)->count == 64 && inv.at(1)->count == 6, "Stack maximum respected");

        ve::InventoryCursor cursor;
        cursor.rightClick(inv, 1);
        require(cursor.carried()->count == 3 && inv.at(1)->count == 3, "Ceil half split");
        cursor.rightClick(inv, 2);
        require(cursor.carried()->count == 2 && inv.at(2)->count == 1, "Single right placement");
        cursor.leftClick(inv, 2);
        require(!cursor.carried() && inv.at(2)->count == 3, "Merge carried stack");

        require(inv.quickMove(0) == 64, "Shift click hotbar to backpack");
        require(!inv.at(0) && inv.at(9)->count == 64, "Quick move destination");
        inv.selectHotbar(8);
        inv.cycleHotbar(1);
        require(inv.selectedHotbar() == 0, "Wheel wrap positive");
        inv.cycleHotbar(-1);
        require(inv.selectedHotbar() == 8, "Wheel wrap negative");

        const ve::ItemStack variant{ ve::ItemId::Dirt, 4, { { "quality", "rare" } } };
        require(inv.insert(variant) == 0, "Metadata insert");
        require(inv.insert(ve::ItemStack{ ve::ItemId::Dirt, 4, { { "quality", "common" } } }) == 0,
                "Distinct metadata must not merge");
        require(inv.at(0)->metadata != inv.at(3)->metadata, "Metadata identities preserved");

        bool rejected = false;
        try {
            inv.insert(ve::ItemStack{ ve::ItemId::WoodenPickaxe, 2 });
        } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "Nonstackable tools must be rejected");
        require(inv.remove(0, 999)->count == 4, "Remove clamps amount");
        require(!inv.at(0), "Last item clears slot");
        std::cout << "[PASS] Inventory: stack, merge, split, click, metadata, quick move, hotbar\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
