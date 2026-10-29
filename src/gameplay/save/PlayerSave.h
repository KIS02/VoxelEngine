#pragma once

#include <filesystem>

#include "gameplay/player/PlayerState.h"


namespace ve
{
    class PlayerSave {
        public:
            static void write( const std::filesystem::path& path, const PlayerState& player );
            static bool read( const std::filesystem::path& path, PlayerState& destination );
    };
}
