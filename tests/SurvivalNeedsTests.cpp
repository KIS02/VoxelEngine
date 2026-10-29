#include <cmath>
#include <iostream>
#include <stdexcept>

#include "gameplay/survival/SurvivalNeeds.h"
#include "gameplay/core/GameplaySession.h"


namespace
{
    void require( bool condition, const char* reason ) {
        if (!condition)
            throw std::runtime_error(reason);
    }
}


int main() {
    try {
        ve::ItemRegistry registry;
        ve::Inventory inventory(registry);
        ve::GameplayEvents events;
        ve::PlayerState player;
        player.initialize(ve::Vec3{ 0.5f, 3.0f, 0.5f });
        player.setMode(ve::GameMode::Survival);
        ve::SurvivalNeeds needs;
        const float before = player.stamina;
        for (int i = 0; i < 60; ++i)
            needs.advance(player, true, 1.0f / 60.0f, events);
        require(player.stamina < before, "Sprint must drain stamina");
        require(player.hunger < player.stats.maxHunger, "Survival must drain hunger");
        for (int i = 0; i < 120; ++i)
            needs.advance(player, false, 1.0f / 60.0f, events);
        require(player.stamina > before - 18.0f, "Stamina must regenerate when not sprinting");

        inventory.set(0, ve::ItemStack{ ve::ItemId::Bread, 2 });
        player.hunger = 5.0f;
        player.health = 10.0f;
        require(needs.eat(player, inventory, events), "Selected food should be consumed");
        require(std::fabs(player.hunger - 11.0f) < 0.01f &&
                std::fabs(player.health - 12.0f) < 0.01f, "Eating did not restore player needs");
        require(inventory.at(0)->count == 1, "Eating must consume exactly one item");
        require(!needs.eat(player, inventory, events), "Food cooldown ignored");

        needs.reset();
        player.health = 4.0f;
        player.hunger = 0.0f;
        for (int i = 0; i < 500; ++i)
            needs.advance(player, false, 1.0f / 60.0f, events);
        require(!player.alive && player.health == 0.0f, "Starvation must cause death");
        int deaths = 0;
        for (const auto& event : events.take())
            deaths += event.type == ve::GameplayEventType::PlayerDied ? 1 : 0;
        require(deaths == 1, "Death event must occur exactly once");
        player.respawn();
        needs.reset();
        require(player.alive && player.health == player.stats.maxHealth, "Respawn must restore HP");
        std::cout << "[PASS] Stamina, hunger, food, starvation, death event, respawn\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
