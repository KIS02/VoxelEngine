#pragma once

#include <filesystem>

#include "gameplay/world/WorldClock.h"


namespace ve
{
    class WorldTimeSave {
        public:
            static void write( const std::filesystem::path& path, const WorldClock& clock );
            static bool read( const std::filesystem::path& path, WorldClock& destination );
    };
}
