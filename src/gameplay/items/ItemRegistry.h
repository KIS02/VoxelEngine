#pragma once

#include <filesystem>
#include <map>

#include "gameplay/items/ItemDefinition.h"


namespace ve
{
    class ItemRegistry {
        private:
            std::map<ItemId, ItemDefinition> definitions_{};


        public:
            ItemRegistry();

            bool contains( ItemId id ) const;
            const ItemDefinition& definition( ItemId id ) const;
            std::size_t size() const;

            // Returns false only when the optional file does not exist.
            // A present but invalid file throws; the previous catalog remains valid.
            bool loadFile( const std::filesystem::path& path );
    };
}
