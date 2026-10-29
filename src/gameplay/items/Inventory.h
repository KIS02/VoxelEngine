#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "gameplay/items/ItemStack.h"


namespace ve
{
    class Inventory {
        private:
            const ItemRegistry* registry_ = nullptr;
            std::vector<std::optional<ItemStack>> slots_{};
            std::size_t selectedHotbar_ = 0;
            std::size_t hotbarSize_ = 9;


        public:
            explicit Inventory( const ItemRegistry& registry, std::size_t slotCount = 36,
                                std::size_t hotbarSize = 9 )
                : registry_(&registry), slots_(slotCount), hotbarSize_(hotbarSize) {
                if (slotCount == 0 || slotCount > 256 || hotbarSize == 0 || hotbarSize > slotCount)
                    throw std::invalid_argument("Invalid inventory dimensions");
            }

            std::size_t size() const {
                return slots_.size();
            }
            std::size_t hotbarSize() const {
                return hotbarSize_;
            }
            std::size_t selectedHotbar() const {
                return selectedHotbar_;
            }
            const ItemRegistry& registry() const {
                return *registry_;
            }

            const std::optional<ItemStack>& at( std::size_t index ) const {
                return slots_.at(index);
            }

            void selectHotbar( std::size_t index ) {
                if (index >= hotbarSize_)
                    throw std::out_of_range("Invalid hotbar index");
                selectedHotbar_ = index;
            }

            void cycleHotbar( int steps ) {
                const int count = static_cast<int>(hotbarSize_);
                const int next = (static_cast<int>(selectedHotbar_) + steps % count + count) % count;
                selectedHotbar_ = static_cast<std::size_t>(next);
            }

            void set( std::size_t index, std::optional<ItemStack> stack ) {
                if (stack)
                    stack->validate(*registry_);
                slots_.at(index) = std::move(stack);
            }

            std::uint16_t insert( const ItemStack& source ) {
                source.validate(*registry_);
                std::uint16_t remaining = source.count;
                const std::uint16_t limit = registry_->definition(source.id).maxStack;

                for (auto& slot : slots_) {
                    if (!slot || !slot->sameKind(source) || slot->count >= limit)
                        continue;
                    const std::uint16_t moved = std::min<std::uint16_t>(remaining, limit - slot->count);
                    slot->count = static_cast<std::uint16_t>(slot->count + moved);
                    remaining = static_cast<std::uint16_t>(remaining - moved);
                    if (remaining == 0)
                        return 0;
                }
                for (auto& slot : slots_) {
                    if (slot)
                        continue;
                    const std::uint16_t moved = std::min<std::uint16_t>(remaining, limit);
                    slot = ItemStack{ source.id, moved, source.metadata };
                    remaining = static_cast<std::uint16_t>(remaining - moved);
                    if (remaining == 0)
                        return 0;
                }
                return remaining;
            }

            std::optional<ItemStack> remove( std::size_t index, std::uint16_t amount ) {
                auto& slot = slots_.at(index);
                if (amount == 0 || !slot)
                    return std::nullopt;
                const std::uint16_t taken = std::min(amount, slot->count);
                ItemStack result{ slot->id, taken, slot->metadata };
                slot->count = static_cast<std::uint16_t>(slot->count - taken);
                if (slot->count == 0)
                    slot.reset();
                return result;
            }

            // Move up to count items from source to destination, respecting item identity.
            std::uint16_t transfer( std::size_t source, std::size_t destination,
                                    std::uint16_t amount ) {
                auto& from = slots_.at(source);
                auto& to = slots_.at(destination);
                if (source == destination || !from || amount == 0)
                    return 0;
                if (to && !to->sameKind(*from))
                    return 0;
                const std::uint16_t limit = registry_->definition(from->id).maxStack;
                const std::uint16_t space = to ? static_cast<std::uint16_t>(limit - to->count) : limit;
                const std::uint16_t moved = std::min({ amount, from->count, space });
                if (moved == 0)
                    return 0;
                if (!to)
                    to = ItemStack{ from->id, moved, from->metadata };
                else
                    to->count = static_cast<std::uint16_t>(to->count + moved);
                from->count = static_cast<std::uint16_t>(from->count - moved);
                if (from->count == 0)
                    from.reset();
                return moved;
            }

            void swap( std::size_t a, std::size_t b ) {
                auto& first = slots_.at(a);
                auto& second = slots_.at(b);
                std::swap(first, second);
            }

            std::uint16_t quickMove( std::size_t source ) {
                if (source >= slots_.size() || !slots_[source])
                    return 0;
                const bool fromHotbar = source < hotbarSize_;
                const std::size_t start = fromHotbar ? hotbarSize_ : 0;
                const std::size_t end = fromHotbar ? slots_.size() : hotbarSize_;
                std::uint16_t moved = 0;
                for (int pass = 0; pass < 2; ++pass) {
                    for (std::size_t i = start; i < end; ++i) {
                        if (!slots_[source])
                            return moved;
                        if ((pass == 0 && !slots_[i]) || (pass == 1 && slots_[i]))
                            continue;
                        moved = static_cast<std::uint16_t>(moved + transfer(source, i, slots_[source]->count));
                    }
                }
                return moved;
            }
    };
}
