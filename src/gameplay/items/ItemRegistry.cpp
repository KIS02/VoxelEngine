#include "gameplay/items/ItemRegistry.h"

#include <set>
#include <stdexcept>
#include <utility>

#include "gameplay/core/ContentTextParser.h"


namespace ve
{
    ItemRegistry::ItemRegistry() {
        definitions_.emplace(ItemId::Grass, ItemDefinition{ ItemId::Grass, "Grass Block", ItemKind::Block, 64 });
        definitions_.emplace(ItemId::Dirt, ItemDefinition{ ItemId::Dirt, "Dirt", ItemKind::Block, 64 });
        definitions_.emplace(ItemId::Stone, ItemDefinition{ ItemId::Stone, "Stone", ItemKind::Block, 64 });
        definitions_.emplace(ItemId::WoodenPickaxe, ItemDefinition{ ItemId::WoodenPickaxe, "Wooden Pickaxe", ItemKind::Tool, 1 });
        definitions_.emplace(ItemId::WoodenAxe, ItemDefinition{ ItemId::WoodenAxe, "Wooden Axe", ItemKind::Tool, 1 });
        definitions_.emplace(ItemId::WoodenShovel, ItemDefinition{ ItemId::WoodenShovel, "Wooden Shovel", ItemKind::Tool, 1 });
        definitions_.emplace(ItemId::CraftingTable, ItemDefinition{ ItemId::CraftingTable, "Crafting Table", ItemKind::Block, 64 });
        definitions_.emplace(ItemId::Bread, ItemDefinition{ ItemId::Bread, "Bread", ItemKind::Food, 64 });
    }

    bool ItemRegistry::contains( ItemId id ) const {
        return definitions_.find(id) != definitions_.end();
    }

    const ItemDefinition& ItemRegistry::definition( ItemId id ) const {
        const auto it = definitions_.find(id);
        if (it == definitions_.end())
            throw std::out_of_range("Unknown ItemId");
        return it->second;
    }

    std::size_t ItemRegistry::size() const {
        return definitions_.size();
    }

    bool ItemRegistry::loadFile( const std::filesystem::path& path ) {
        auto next = definitions_;
        std::set<ItemId> seen;
        const bool loaded = content::readLines(path, [&]( const std::vector<std::string>& fields ) {
            if (fields.size() != 4)
                throw std::invalid_argument("Expected id|name|kind|maxStack");

            const int numericId = content::parseInt(fields[0]);
            if (numericId <= 0 || numericId > 65535 || fields[1].empty())
                throw std::invalid_argument("Invalid ItemId or empty name");

            const ItemId id = static_cast<ItemId>(numericId);
            if (next.find(id) == next.end())
                throw std::invalid_argument("Unknown ItemId; explicit C++ ID registration required");
            if (!seen.insert(id).second)
                throw std::invalid_argument("Duplicate ItemId");

            ItemKind kind;
            if (fields[2] == "block")
                kind = ItemKind::Block;
            else if (fields[2] == "tool")
                kind = ItemKind::Tool;
            else if (fields[2] == "food")
                kind = ItemKind::Food;
            else
                throw std::invalid_argument("Invalid item kind");

            const int limit = content::parseInt(fields[3]);
            if (limit < 1 || limit > 99 || (kind == ItemKind::Tool && limit != 1))
                throw std::invalid_argument("Invalid item stack limit");
            if (next.at(id).kind != kind)
                throw std::invalid_argument("Changing item kind would violate its stable ID contract");

            // Reject duplicate IDs even when their text would be identical.
            // Per-file IDs are tracked in the load closure.
            next.at(id) = ItemDefinition{ id, fields[1], kind, static_cast<std::uint16_t>(limit) };
        });

        if (loaded)
            definitions_.swap(next);
        return loaded;
    }
}
