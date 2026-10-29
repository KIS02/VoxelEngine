#include "gameplay/block/GameplayBlockCatalog.h"

#include <set>
#include <stdexcept>

#include "gameplay/core/ContentTextParser.h"


namespace ve
{
    GameplayBlockCatalog::GameplayBlockCatalog() {
        definitions_.emplace(BlockId::Air, GameplayBlockDefinition{ BlockRegistry::definition(BlockId::Air), "Air", 0.0f, ItemId::None });
        definitions_.emplace(BlockId::Grass, GameplayBlockDefinition{ BlockRegistry::definition(BlockId::Grass), "Grass", 0.6f, ItemId::Dirt });
        definitions_.emplace(BlockId::Dirt, GameplayBlockDefinition{ BlockRegistry::definition(BlockId::Dirt), "Dirt", 0.5f, ItemId::Dirt });
        definitions_.emplace(BlockId::Stone, GameplayBlockDefinition{ BlockRegistry::definition(BlockId::Stone), "Stone", 1.5f, ItemId::Stone });
        definitions_.emplace(BlockId::CraftingTable, GameplayBlockDefinition{ BlockRegistry::definition(BlockId::CraftingTable), "Crafting Table", 1.2f, ItemId::CraftingTable });
    }

    const GameplayBlockDefinition& GameplayBlockCatalog::definition( BlockId id ) const {
        const auto it = definitions_.find(id);
        if (it == definitions_.end())
            throw std::out_of_range("Unknown BlockId");
        return it->second;
    }

    std::size_t GameplayBlockCatalog::size() const {
        return definitions_.size();
    }

    bool GameplayBlockCatalog::loadFile( const std::filesystem::path& path ) {
        auto next = definitions_;
        std::set<BlockId> seen;
        const bool loaded = content::readLines(path, [&]( const std::vector<std::string>& fields ) {
            if (fields.size() != 4)
                throw std::invalid_argument("Expected id|name|hardness|dropItemId");

            const int numericId = content::parseInt(fields[0]);
            if (numericId < 0 || numericId > 255 || fields[1].empty())
                throw std::invalid_argument("Invalid BlockId or empty name");

            const BlockId id = static_cast<BlockId>(numericId);
            if (next.find(id) == next.end())
                throw std::invalid_argument("Unknown BlockId; existing world IDs must remain stable");
            if (!seen.insert(id).second)
                throw std::invalid_argument("Duplicate BlockId");

            const float hardness = content::parseFloat(fields[2]);
            if (hardness < 0.0f || (id != BlockId::Air && hardness <= 0.0f))
                throw std::invalid_argument("Invalid block hardness");

            const int numericDrop = content::parseInt(fields[3]);
            if (numericDrop != 0 && numericDrop != 1 && numericDrop != 2 && numericDrop != 3 && numericDrop != 103)
                throw std::invalid_argument("Unknown block drop ItemId");
            if (id == BlockId::Air && (hardness != 0.0f || numericDrop != 0))
                throw std::invalid_argument("Air must have no hardness or drop");

            auto& entry = next.at(id);
            entry.name = fields[1];
            entry.hardness = hardness;
            entry.drop = static_cast<ItemId>(numericDrop);
        });

        if (loaded)
            definitions_.swap(next);
        return loaded;
    }
}
