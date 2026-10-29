#pragma once

#include <cstdint>
#include <filesystem>

#include "gameplay/items/Inventory.h"
#include "gameplay/items/WorldItems.h"


namespace ve
{
    class InventorySave {
        public:
            static void write( const std::filesystem::path& path, const Inventory& inventory,
                               const WorldItems* world = nullptr );
            // A missing file is not an error. Invalid files throw without mutating inventory.
            static bool read( const std::filesystem::path& path, Inventory& inventory,
                              WorldItems* world = nullptr );
    };
}
