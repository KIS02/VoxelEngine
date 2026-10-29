#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>

#include "gameplay/core/GameMode.h"
#include "gameplay/player/PlayerStats.h"
#include "math/Vec3.h"


namespace ve
{
    struct PlayerState
    {
        std::uint64_t id = 1;
        Vec3 position{};
        Vec3 previousPosition{};
        Vec3 velocity{};
        Vec3 spawnPosition{};
        PlayerStats stats{};
        GameMode mode = GameMode::Creative;
        float health = 20.0f;
        float hunger = 20.0f;
        float stamina = 100.0f;
        bool grounded = false;
        bool alive = true;
        bool spawned = false;


        static bool finitePosition( const Vec3& value ) {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
        }

        void setPosition( const Vec3& value ) {
            if (!finitePosition(value))
                throw std::invalid_argument("Player position must be finite");

            previousPosition = position;
            position = value;
        }

        void initialize( const Vec3& spawn ) {
            stats.validate();
            if (!finitePosition(spawn))
                throw std::invalid_argument("Player spawn must be finite");

            spawnPosition = spawn;
            position = spawn;
            previousPosition = spawn;
            velocity = Vec3{};
            health = stats.maxHealth;
            hunger = stats.maxHunger;
            stamina = stats.maxStamina;
            grounded = false;
            alive = true;
            spawned = true;
        }

        void respawn() {
            if (!spawned)
                throw std::logic_error("Cannot respawn before the initial spawn");

            initialize(spawnPosition);
        }

        void setMode( GameMode next ) {
            mode = next;
            velocity = Vec3{};
            grounded = false;
        }
    };
}
