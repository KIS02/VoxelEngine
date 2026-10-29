#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>

#include "gameplay/items/ItemRegistry.h"


namespace ve
{
    struct ItemStack
    {
        ItemId id = ItemId::None;
        std::uint16_t count = 0;
        std::map<std::string, std::string> metadata{};


        bool empty() const {
            return id == ItemId::None || count == 0;
        }

        bool sameKind( const ItemStack& other ) const {
            return id == other.id && metadata == other.metadata;
        }

        void validate( const ItemRegistry& registry ) const {
            if (id == ItemId::None || !registry.contains(id) || count == 0 ||
                count > registry.definition(id).maxStack || metadata.size() > 16)
                throw std::invalid_argument("Invalid ItemStack ID, count or metadata size");

            for (const auto& [key, value] : metadata) {
                if (key.empty() || key.size() > 48 || value.size() > 256)
                    throw std::invalid_argument("Invalid ItemStack metadata field");
            }
        }
    };
}
