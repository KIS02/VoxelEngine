#pragma once

#include <cstdint>
#include <string>

#include "gameplay/core/GameMode.h"
#include "math/Vec3.h"


namespace ve
{
    enum class GameplayEventType {
        PlayerSpawned,
        PlayerRespawned,
        PlayerMoved,
        GameModeChanged,
        BlockBroken,
        BlockPlaced,
        PlayerJumped,
        PlayerLanded,
        PlayerFallDamaged,
        ItemPickedUp,
        ItemDropped,
        ItemCrafted,
        ToolBroken,
        PlayerDamaged,
        PlayerDied,
        FoodConsumed,
        WorldDayChanged,
        CooldownExpired
    };


    struct GameplayEvent
    {
        GameplayEventType type = GameplayEventType::PlayerSpawned;
        std::uint64_t playerId = 0;
        Vec3 position{};
        GameMode mode = GameMode::Creative;
        std::string subject{};
    };
}
