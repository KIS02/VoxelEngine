#pragma once

#include <optional>
#include <sstream>
#include <string>

#include "gameplay/crafting/CraftingGrid.h"
#include "ui/UIFrameData.h"


namespace ve
{
    class CraftingHud {
        public:
            static std::optional<std::size_t> slotAt( int x, int y, const CraftingGrid& crafting ) {
                const int dx = x - 20;
                const int dy = y - 225;
                const int width = crafting.width();
                if (dx < 0 || dx >= width * 80 || dy < 0 || dy >= width * 15)
                    return std::nullopt;
                return static_cast<std::size_t>((dy / 15) * width + (dx / 80));
            }

            static bool resultAt( int x, int y, const CraftingGrid& crafting ) {
                const int top = 225 + crafting.width() * 15;
                return x >= 20 && x < 350 && y >= top && y < top + 15;
            }

            static void append( UIFrameData& ui, const CraftingGrid& crafting, bool open ) {
                if (!open || !ui.inventoryOverlay.enabled)
                    return;
                ui.inventoryOverlay.lines.push_back("CRAFTING " + std::to_string(crafting.width()) + "x" +
                    std::to_string(crafting.width()) + "   LEFT/RIGHT: INPUT    OUTPUT CLICK: CRAFT");
                for (int row = 0; row < crafting.width(); ++row) {
                    std::string line;
                    for (int col = 0; col < crafting.width(); ++col) {
                        const auto& stack = crafting.slots().at(static_cast<std::size_t>(row * crafting.width() + col));
                        if (!stack) {
                            line += "[------]";
                            continue;
                        }
                        std::ostringstream item;
                        item << '[' << static_cast<int>(stack->id) << ':' << stack->count << ']';
                        std::string cell = item.str();
                        cell.resize(8, ' ');
                        line += cell;
                    }
                    ui.inventoryOverlay.lines.push_back(line);
                }
                const auto product = crafting.preview();
                if (product)
                    ui.inventoryOverlay.lines.push_back("RESULT: " +
                        crafting.slots().registry().definition(product->id).name + " x" +
                        std::to_string(product->count) + "  [CLICK HERE]");
                else
                    ui.inventoryOverlay.lines.push_back("RESULT: (no matching recipe)");
                ui.inventoryOverlay.lines.push_back("RECIPES: 4 DIRT=GRASS, 4 STONE=TABLE, 2 GRASS=BREAD; 3x3 TOOLS");
            }
    };
}
