#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "gameplay/combat/DamageSystem.h"
#include "gameplay/items/Inventory.h"


namespace ve
{
    class SurvivalNeeds {
        private:
            float starvationSeconds_ = 0.0f;
            float foodCooldown_ = 0.0f;


        public:
            void reset() {
                starvationSeconds_ = 0.0f;
                foodCooldown_ = 0.0f;
            }

            void advance( PlayerState& player, bool sprinting, float dt, GameplayEvents& events ) {
                if (!std::isfinite(dt) || dt <= 0.0f || dt > 0.05f)
                    throw std::invalid_argument("Invalid survival step");
                foodCooldown_ = std::max(0.0f, foodCooldown_ - dt);
                if (!player.alive || player.mode != GameMode::Survival)
                    return;

                player.hunger = std::max(0.0f, player.hunger - (0.025f + (sprinting ? 0.10f : 0.0f)) * dt);
                player.stamina = std::clamp(player.stamina + (sprinting ? -18.0f : 12.0f) * dt,
                    0.0f, player.stats.maxStamina);
                if (player.hunger > 0.0f) {
                    starvationSeconds_ = 0.0f;
                    return;
                }
                starvationSeconds_ += dt;
                if (starvationSeconds_ + 1.0e-5f >= 4.0f) {
                    starvationSeconds_ -= 4.0f;
                    DamageSystem::apply(player, 2.0f, DamageType::Starvation, events);
                }
            }

            bool eat( PlayerState& player, Inventory& inventory, GameplayEvents& events ) {
                if (!player.alive || player.mode != GameMode::Survival || foodCooldown_ > 0.0f)
                    return false;
                const std::size_t slot = inventory.selectedHotbar();
                const auto& selected = inventory.at(slot);
                if (!selected || inventory.registry().definition(selected->id).kind != ItemKind::Food)
                    return false;
                if (player.hunger >= player.stats.maxHunger && player.health >= player.stats.maxHealth)
                    return false;
                const auto consumed = inventory.remove(slot, 1);
                if (!consumed)
                    return false;

                player.hunger = std::min(player.stats.maxHunger, player.hunger + 6.0f);
                player.health = std::min(player.stats.maxHealth, player.health + 2.0f);
                foodCooldown_ = 0.8f;
                starvationSeconds_ = 0.0f;
                events.publish(GameplayEvent{ GameplayEventType::FoodConsumed,
                    player.id, player.position, player.mode, std::to_string(static_cast<int>(consumed->id)) });
                return true;
            }
    };
}
