#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

#include "gameplay/items/Inventory.h"
#include "voxel/block/BlockId.h"


namespace ve
{
    class ToolRules {
        public:
            static constexpr std::uint16_t MaxDurability = 64;

            static float miningSpeed( BlockId block, const std::optional<ItemStack>& held ) {
                if (!held)
                    return 1.0f;
                const bool isTool = held->id == ItemId::WoodenPickaxe ||
                    held->id == ItemId::WoodenAxe || held->id == ItemId::WoodenShovel;
                if (isTool && durability(*held) == 0)
                    return 1.0f;
                if (block == BlockId::Stone && held->id == ItemId::WoodenPickaxe)
                    return 4.0f;
                if ((block == BlockId::Dirt || block == BlockId::Grass) && held->id == ItemId::WoodenShovel)
                    return 4.0f;
                if (block == BlockId::CraftingTable && held->id == ItemId::WoodenAxe)
                    return 4.0f;
                return 1.0f;
            }

            static std::uint16_t durability( const ItemStack& stack ) {
                const auto it = stack.metadata.find("durability");
                if (it == stack.metadata.end())
                    return MaxDurability;
                if (it->second.empty() || it->second.size() > 3)
                    throw std::invalid_argument("Invalid tool durability metadata");
                unsigned int value = 0;
                for (char digit : it->second) {
                    if (digit < '0' || digit > '9')
                        throw std::invalid_argument("Non-numeric tool durability");
                    value = value * 10 + static_cast<unsigned int>(digit - '0');
                }
                if (value > MaxDurability)
                    throw std::invalid_argument("Tool durability out of range");
                return static_cast<std::uint16_t>(value);
            }

            static bool consumeUse( Inventory& inventory, std::size_t slot ) {
                const auto value = inventory.at(slot);
                if (!value || inventory.registry().definition(value->id).kind != ItemKind::Tool)
                    return false;
                ItemStack changed = *value;
                const std::uint16_t current = durability(changed);
                if (current <= 1) {
                    inventory.set(slot, std::nullopt);
                    return true;
                }
                changed.metadata["durability"] = std::to_string(current - 1);
                inventory.set(slot, changed);
                return false;
            }
    };
}
