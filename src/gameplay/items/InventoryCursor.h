#pragma once

#include <optional>
#include <utility>

#include "gameplay/items/Inventory.h"


namespace ve
{
    class InventoryCursor {
        private:
            std::optional<ItemStack> carried_{};


        public:
            const std::optional<ItemStack>& carried() const {
                return carried_;
            }
            void restore( std::optional<ItemStack> item ) { carried_ = std::move(item); }

            // Left-click picks up all, places all, merges or swaps.
            void leftClick( Inventory& inventory, std::size_t slot ) {
                auto target = inventory.at(slot);
                if (!carried_) {
                    if (target) {
                        carried_ = target;
                        inventory.set(slot, std::nullopt);
                    }
                    return;
                }
                if (!target) {
                    inventory.set(slot, carried_);
                    carried_.reset();
                    return;
                }
                if (target->sameKind(*carried_)) {
                    const std::uint16_t limit = inventory.registry().definition(target->id).maxStack;
                    const std::uint16_t moved = std::min<std::uint16_t>(carried_->count, limit - target->count);
                    target->count = static_cast<std::uint16_t>(target->count + moved);
                    carried_->count = static_cast<std::uint16_t>(carried_->count - moved);
                    inventory.set(slot, target);
                    if (carried_->count == 0)
                        carried_.reset();
                    return;
                }
                inventory.set(slot, carried_);
                carried_ = std::move(target);
            }

            // Right-click splits the source by ceil(n/2) or places one item.
            void rightClick( Inventory& inventory, std::size_t slot ) {
                auto target = inventory.at(slot);
                if (!carried_) {
                    if (target) {
                        const std::uint16_t amount = static_cast<std::uint16_t>((target->count + 1) / 2);
                        carried_ = inventory.remove(slot, amount);
                    }
                    return;
                }
                if (!target || target->sameKind(*carried_)) {
                    const std::uint16_t limit = inventory.registry().definition(carried_->id).maxStack;
                    if (target && target->count >= limit)
                        return;
                    if (target)
                        ++target->count;
                    else
                        target = ItemStack{ carried_->id, 1, carried_->metadata };
                    inventory.set(slot, target);
                    if (--carried_->count == 0)
                        carried_.reset();
                }
            }

            // Preserve items when closing the UI: hand contents must have a destination.
            bool putAway( Inventory& inventory ) {
                if (!carried_)
                    return true;
                const std::uint16_t remaining = inventory.insert(*carried_);
                if (remaining == 0)
                    carried_.reset();
                else
                    carried_->count = remaining;
                return !carried_;
            }
    };
}
