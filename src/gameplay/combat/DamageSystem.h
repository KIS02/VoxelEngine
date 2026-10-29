#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

#include "gameplay/core/GameplayEvents.h"
#include "gameplay/player/PlayerState.h"


namespace ve
{
    enum class DamageType {
        Fall,
        Starvation,
        Generic
    };


    struct DamageResult
    {
        float applied = 0.0f;
        bool killed = false;
    };


    class DamageSystem {
        public:
            static DamageResult apply( PlayerState& player, float amount, DamageType type,
                                       GameplayEvents& events ) {
                if (!std::isfinite(amount) || amount < 0.0f)
                    throw std::invalid_argument("Invalid damage amount");
                if (!player.alive || player.mode != GameMode::Survival || amount == 0.0f)
                    return DamageResult{};

                const float before = player.health;
                player.health = std::max(0.0f, before - amount);
                const float applied = before - player.health;
                const char* reason = type == DamageType::Fall ? "fall" :
                    type == DamageType::Starvation ? "starvation" : "generic";
                if (applied > 0.0f)
                    events.publish(GameplayEvent{ GameplayEventType::PlayerDamaged, player.id,
                        player.position, player.mode, std::string(reason) + ":" + std::to_string(applied) });
                if (type == DamageType::Fall && applied > 0.0f)
                    events.publish(GameplayEvent{ GameplayEventType::PlayerFallDamaged, player.id,
                        player.position, player.mode, std::to_string(applied) });

                const bool killed = player.health == 0.0f;
                if (killed) {
                    player.alive = false;
                    player.velocity = Vec3{};
                    events.publish(GameplayEvent{ GameplayEventType::PlayerDied, player.id,
                        player.position, player.mode, reason });
                }
                return DamageResult{ applied, killed };
            }
    };
}
