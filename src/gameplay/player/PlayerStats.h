#pragma once

#include <cmath>
#include <stdexcept>


namespace ve
{
    struct PlayerStats
    {
        float maxHealth = 20.0f;
        float maxHunger = 20.0f;
        float maxStamina = 100.0f;
        float walkSpeed = 4.5f;
        float sprintMultiplier = 1.5f;
        float jumpSpeed = 7.0f;


        void validate() const {
            if (!std::isfinite(maxHealth) || maxHealth <= 0.0f ||
                !std::isfinite(maxHunger) || maxHunger <= 0.0f ||
                !std::isfinite(maxStamina) || maxStamina <= 0.0f ||
                !std::isfinite(walkSpeed) || walkSpeed <= 0.0f ||
                !std::isfinite(sprintMultiplier) || sprintMultiplier < 1.0f ||
                !std::isfinite(jumpSpeed) || jumpSpeed <= 0.0f)
                throw std::invalid_argument("Invalid PlayerStats");
        }
    };
}
