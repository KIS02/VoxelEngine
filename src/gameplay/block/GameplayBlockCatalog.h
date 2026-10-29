#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "assets/block/BlockRegistry.h"
#include "gameplay/items/ItemDefinition.h"


namespace ve
{
    struct GameplayBlockDefinition
    {
        BlockDefinition visual{};
        std::string name{};
        float hardness = 0.0f;
        ItemId drop = ItemId::None;
    };


    class GameplayBlockCatalog {
        private:
            std::map<BlockId, GameplayBlockDefinition> definitions_{};


        public:
            GameplayBlockCatalog();

            const GameplayBlockDefinition& definition( BlockId id ) const;
            std::size_t size() const;
            bool loadFile( const std::filesystem::path& path );
    };
}
