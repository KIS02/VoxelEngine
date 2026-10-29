#pragma once

#include <stdexcept>

#include "gameplay/player/PlayerState.h"


namespace ve
{
    class PlayerSpawn {
        public:
            static void setSpawnPoint( PlayerState& player, const Vec3& position ) {
                if (!PlayerState::finitePosition(position))
                    throw std::invalid_argument("Invalid spawn point");

                player.spawnPosition = position;
            }

            static void spawn( PlayerState& player, const Vec3& position ) {
                player.initialize(position);
            }

            static void respawn( PlayerState& player ) {
                player.respawn();
            }
    };
}
