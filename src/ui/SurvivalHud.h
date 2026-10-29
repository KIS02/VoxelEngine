#pragma once

#include <algorithm>
#include <cmath>
#include <string>

#include "gameplay/player/PlayerState.h"
#include "ui/UIFrameData.h"


namespace ve
{
    class SurvivalHud {
        private:
            static std::string meter( const char* name, float value, float maximum, int width ) {
                const int filled = std::clamp(static_cast<int>(std::round(value / maximum * width)), 0, width);
                return std::string(name) + " [" + std::string(static_cast<std::size_t>(filled), '#') +
                    std::string(static_cast<std::size_t>(width - filled), '-') + "] " +
                    std::to_string(static_cast<int>(std::round(value))) + "/" +
                    std::to_string(static_cast<int>(std::round(maximum)));
            }


        public:
            static void append( UIFrameData& ui, const PlayerState& player ) {
                if (player.mode != GameMode::Survival)
                    return;
                ui.hotbarOverlay.enabled = true;
                ui.hotbarOverlay.originY = std::max(8, ui.hotbarOverlay.originY - 65);
                ui.hotbarOverlay.lines.push_back(meter("HP", player.health, player.stats.maxHealth, 10));
                ui.hotbarOverlay.lines.push_back(meter("HUNGER", player.hunger, player.stats.maxHunger, 10));
                ui.hotbarOverlay.lines.push_back(meter("STAMINA", player.stamina, player.stats.maxStamina, 10));
                if (!player.alive)
                    ui.hotbarOverlay.lines.push_back("YOU DIED  -  PRESS R TO RESPAWN");
                else
                    ui.hotbarOverlay.lines.push_back("F: EAT SELECTED FOOD");
            }
    };
}
