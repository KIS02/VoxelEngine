#include <iostream>
#include <stdexcept>
#include <string>

#include "ui/SurvivalHud.h"
#include "input/ApplicationInput.h"


namespace
{
    void require( bool value, const char* reason ) {
        if (!value)
            throw std::runtime_error(reason);
    }
}


int main() {
    try {
        ve::PlayerState player;
        player.initialize(ve::Vec3{});
        ve::UIFrameData ui;
        ve::SurvivalHud::append(ui, player);
        require(ui.hotbarOverlay.lines.empty(), "Creative must not display survival meters");
        player.setMode(ve::GameMode::Survival);
        ve::SurvivalHud::append(ui, player);
        require(ui.hotbarOverlay.lines.size() == 4, "Health/hunger/stamina HUD missing");
        player.health = 0.0f;
        player.alive = false;
        ve::UIFrameData deadUi;
        ve::SurvivalHud::append(deadUi, player);
        require(deadUi.hotbarOverlay.lines.back().find("RESPAWN") != std::string::npos,
                "Death HUD must show respawn control");

        ve::ApplicationInput input;
        ve::InputSnapshot snapshot;
        snapshot.consumeFood = true;
        require(input.process(snapshot, true).consumeFood, "F rising edge missing");
        require(!input.process(snapshot, true).consumeFood, "Holding F repeats food action");
        snapshot.consumeFood = false;
        input.process(snapshot, true);
        snapshot.consumeFood = true;
        require(!input.process(snapshot, false, true).consumeFood,
                "Inventory UI must consume food input");
        snapshot.consumeFood = false;
        input.process(snapshot, true);
        snapshot.consumeFood = true;
        require(input.process(snapshot, true).consumeFood, "F re-arm failed");
        std::cout << "[PASS] Survival HUD, death HUD, food input focus and edge\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
