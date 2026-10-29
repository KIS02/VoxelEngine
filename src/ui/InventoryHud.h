#pragma once

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>

#include "gameplay/items/Inventory.h"
#include "gameplay/items/InventoryCursor.h"
#include "ui/UIFrameData.h"


namespace ve
{
    class InventoryHud {
        private:
            static constexpr int CellWidth = 80;
            static constexpr int LineHeight = 15;
            static constexpr int PanelX = 12;
            static constexpr int PanelY = 90;


            static std::string itemCode( ItemId item ) {
                switch (item) {
                    case ItemId::Grass: return "GRS";
                    case ItemId::Dirt: return "DRT";
                    case ItemId::Stone: return "STN";
                    case ItemId::WoodenPickaxe: return "PIC";
                    case ItemId::WoodenAxe: return "AXE";
                    case ItemId::WoodenShovel: return "SHV";
                    case ItemId::CraftingTable: return "TBL";
                    case ItemId::Bread: return "BRD";
                    default: return "???";
                }
            }

            static std::string cell( const std::optional<ItemStack>& item, bool selected ) {
                const char begin = selected ? '(' : '[';
                const char end = selected ? ')' : ']';
                if (!item)
                    return std::string(1, begin) + "------" + end;
                std::ostringstream text;
                text << begin << itemCode(item->id) << std::setfill('0')
                     << std::setw(3) << item->count << end;
                return text.str();
            }

            static std::string row( const Inventory& inv, std::size_t first, bool highlight ) {
                std::string text;
                for (std::size_t col = 0; col < 9; ++col) {
                    const std::size_t slot = first + col;
                    text += cell(inv.at(slot), highlight && slot == inv.selectedHotbar());
                }
                return text;
            }

        public:
            static std::optional<std::size_t> slotAt( int x, int y ) {
                const int localX = x - (PanelX + 8);
                const int localY = y - (PanelY + 8 + LineHeight);
                if (localX < 0 || localX >= 9 * CellWidth || localY < 0 || localY >= 4 * LineHeight)
                    return std::nullopt;
                const int column = localX / CellWidth;
                const int line = localY / LineHeight;
                if (line == 3)
                    return static_cast<std::size_t>(column);
                return static_cast<std::size_t>(9 + line * 9 + column);
            }

            static void append( UIFrameData& ui, const Inventory& inventory,
                                const InventoryCursor& cursor, bool open, int width, int height,
                                int pointerX = -1, int pointerY = -1 ) {
                if (width < 748 || height < 250)
                    return;
                ui.hotbarOverlay.enabled = true;
                ui.hotbarOverlay.originX = std::max(0, (width - 736) / 2);
                ui.hotbarOverlay.originY = height - 40;
                ui.hotbarOverlay.lines = { "HOTBAR [1..9]  E: INVENTORY  Q: DROP", row(inventory, 0, true) };

                if (!open)
                    return;
                ui.crosshair.enabled = false;
                ui.inventoryOverlay.enabled = true;
                ui.inventoryOverlay.originX = PanelX;
                ui.inventoryOverlay.originY = PanelY;
                ui.inventoryOverlay.lines = {
                    "INVENTORY   E / ESC: CLOSE  SHIFT+CLICK: MOVE",
                    row(inventory, 9, false), row(inventory, 18, false), row(inventory, 27, false),
                    row(inventory, 0, true),
                    "LEFT: PICK / MERGE / SWAP    RIGHT: HALF / ONE"
                };
                if (cursor.carried()) {
                    const ItemStack& carried = *cursor.carried();
                    ui.inventoryOverlay.lines.push_back("HAND: " + inventory.registry().definition(carried.id).name +
                        " x" + std::to_string(carried.count));
                } else {
                    ui.inventoryOverlay.lines.push_back("HAND: EMPTY");
                }
                const auto hovered = slotAt(pointerX, pointerY);
                if (hovered && inventory.at(*hovered)) {
                    const ItemStack& stack = *inventory.at(*hovered);
                    ui.inventoryOverlay.lines.push_back("ITEM: " + inventory.registry().definition(stack.id).name +
                        " x" + std::to_string(stack.count));
                } else {
                    ui.inventoryOverlay.lines.push_back("ITEM: NONE");
                }
            }
    };
}
