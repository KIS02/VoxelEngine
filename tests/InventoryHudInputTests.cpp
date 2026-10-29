#include <iostream>
#include <stdexcept>

#include "input/ApplicationInput.h"
#include "ui/InventoryHud.h"


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
        ve::InventoryCursor cursor;
        ve::UIFrameData ui{};
        ve::InventoryHud::append(ui, inv, cursor, true, 800, 600);
        require(ui.inventoryOverlay.enabled && ui.hotbarOverlay.enabled, "Both UI panels available");
        require(ve::InventoryHud::slotAt(20, 113).value() == 9, "Backpack row one");
        require(ve::InventoryHud::slotAt(20, 158).value() == 0, "Hotbar row four");
        require(ve::InventoryHud::slotAt(20 + 8 * 80, 113).value() == 17, "Backpack last column");
        require(!ve::InventoryHud::slotAt(-1, 0).has_value(), "Outside window rejected");

        ve::ApplicationInput input;
        ve::InputSnapshot snap{};
        snap.toggleInventory = true;
        require(input.process(snap, true, false).toggleInventory, "Inventory toggle edge");
        require(!input.process(snap, false, true).toggleInventory, "No repeated toggle while held");
        snap.toggleInventory = false;
        snap.leftMouse = true;
        const ve::InputActions click = input.process(snap, false, true);
        require(click.inventoryLeft && !click.breakBlock && !click.captureCursor,
                "Inventory click never breaks blocks or captures cursor");
        snap.leftMouse = false;
        snap.escape = true;
        const auto escape = input.process(snap, false, true);
        require(escape.closeInventory && !escape.requestExit, "ESC closes inventory, not game");
        std::cout << "[PASS] Inventory HUD and Input focus contract\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
