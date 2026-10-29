#include <iostream>
#include <stdexcept>

#include "input/ApplicationInput.h"
#include "ui/InventoryHud.h"
#include "ui/CraftingHud.h"


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
        ve::CraftingGrid crafting(items);
        ve::Inventory inventory(items);
        ve::InventoryCursor cursor;
        ve::UIFrameData ui{};
        ve::InventoryHud::append(ui, inventory, cursor, true, 800, 600);
        ve::CraftingHud::append(ui, crafting, true);
        require(ui.inventoryOverlay.lines.size() == 13, "2x2 UI lines have fixed layout");
        require(ve::CraftingHud::slotAt(20, 225, crafting).value() == 0, "2x2 top-left slot");
        require(ve::CraftingHud::slotAt(100, 240, crafting).value() == 3, "2x2 bottom-right slot");
        require(!ve::CraftingHud::slotAt(180, 225, crafting), "2x2 excludes third column");
        require(ve::CraftingHud::resultAt(20, 255, crafting), "2x2 output button");
        crafting.setWorkbench(true);
        require(ve::CraftingHud::slotAt(180, 255, crafting).value() == 8, "3x3 third row third column");
        require(ve::CraftingHud::resultAt(20, 270, crafting), "3x3 output button");

        ve::ApplicationInput input;
        ve::InputSnapshot snapshot{};
        snapshot.leftMouse = true;
        const auto actions = input.process(snapshot, false, true);
        require(actions.inventoryLeft && !actions.breakHeld && !actions.breakBlock,
                "Crafting clicks must not mine, walk, or capture the cursor");
        std::cout << "[PASS] 2x2/3x3 UI, output button, block input isolation\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
