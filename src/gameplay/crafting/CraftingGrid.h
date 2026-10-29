#pragma once

#include <array>
#include <optional>
#include <stdexcept>
#include <cstdint>
#include <vector>
#include <utility>

#include "gameplay/crafting/RecipeRegistry.h"
#include "gameplay/items/Inventory.h"
#include "gameplay/items/InventoryCursor.h"
#include "gameplay/items/WorldItems.h"


namespace ve
{
    class CraftingGrid {
        private:
            Inventory slots_;
            RecipeRegistry recipes_{};
            int width_ = 2;

        public:
            explicit CraftingGrid( const ItemRegistry& items ) : slots_(items, 9, 9) { }

            const Inventory& slots() const { return slots_; }
            Inventory& slots() { return slots_; }
            const RecipeRegistry& recipes() const { return recipes_; }
            RecipeRegistry& recipes() { return recipes_; }
            int width() const { return width_; }

            void setWorkbench( bool enabled ) {
                if (width_ == (enabled ? 3 : 2))
                    return;
                for (int i = 0; i < 9; ++i) {
                    if (slots_.at(i))
                        throw std::logic_error("Clear crafting input before changing workstation");
                }
                width_ = enabled ? 3 : 2;
            }

            std::optional<ItemStack> preview() const {
                std::array<std::optional<ItemStack>, 9> grid{};
                for (int i = 0; i < width_ * width_; ++i)
                    grid[i] = slots_.at(static_cast<std::size_t>(i));
                return recipes_.match(grid, width_);
            }

            bool craft( Inventory& inventory ) {
                const auto output = preview();
                if (!output)
                    return false;

                // Copy the inventory to guarantee that a full output never consumes ingredients.
                Inventory destination = inventory;
                if (destination.insert(*output) != 0)
                    return false;

                for (int i = 0; i < width_ * width_; ++i) {
                    if (slots_.at(static_cast<std::size_t>(i)))
                        slots_.remove(static_cast<std::size_t>(i), 1);
                }
                // Inventory copies retain the same non-owning registry pointer.
                inventory = std::move(destination);
                return true;
            }

            void putAway( Inventory& inventory, WorldItems& world, const Vec3& position ) {
                Inventory destination = inventory;
                std::vector<std::uint64_t> spawned;
                try {
                    for (std::size_t i = 0; i < slots_.size(); ++i) {
                        if (!slots_.at(i))
                            continue;
                        ItemStack current = *slots_.at(i);
                        const std::uint16_t remainder = destination.insert(current);
                        if (remainder != 0) {
                            current.count = remainder;
                            spawned.push_back(world.spawn(current, position));
                        }
                    }
                } catch (...) {
                    for (std::uint64_t id : spawned)
                        world.remove(id);
                    throw;
                }

                inventory = std::move(destination);
                for (std::size_t i = 0; i < slots_.size(); ++i)
                    slots_.set(i, std::nullopt);
            }
    };
}
