#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "gameplay/core/GameplaySession.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        ve::GameplaySession gameplay;
        gameplay.start(ve::Vec3{ 1.0f, 2.0f, 3.0f });
        gameplay.inventory().set(0, ve::ItemStack{ ve::ItemId::Dirt, 10 });

        require(gameplay.dropSelected(ve::Vec3{ 1.0f, 0.0f, 0.0f }),
                "Q drops selected item");
        require(gameplay.inventory().at(0).has_value() && gameplay.inventory().at(0)->count == 9,
                "Drop consumed exactly one");
        require(gameplay.worldItems().all().size() == 1, "Spawned world drop");

        const ve::Vec3 droppedPosition = gameplay.worldItems().all().front().position;

        bool seenDrop = false;
        for (const auto& event : gameplay.events().take())
            seenDrop |= event.type == ve::GameplayEventType::ItemDropped;
        require(seenDrop, "Drop must publish an event");

        const float nan = std::numeric_limits<float>::quiet_NaN();
        bool rejected = false;
        try {
            gameplay.dropSelected(ve::Vec3{ nan, 0.0f, 0.0f });
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && gameplay.inventory().at(0).has_value() &&
                gameplay.inventory().at(0)->count == 9 && gameplay.worldItems().all().size() == 1,
                "Failed spawn must roll back the entire slot");

        // The player starts within pickup range, but the initial delay prevents collection.
        for (int tick = 0; tick < 30; ++tick)
            gameplay.advance(1.0f / 60.0f);
        require(gameplay.worldItems().all().size() == 1 &&
                gameplay.inventory().at(0).has_value() && gameplay.inventory().at(0)->count == 9,
                "Pickup delay must prevent early collection");

        // Let the delay expire while outside pickup range.
        gameplay.player().setPosition(ve::Vec3{ 100.0f, 2.0f, 100.0f });
        for (int tick = 0; tick < 60; ++tick)
            gameplay.advance(1.0f / 60.0f);
        require(gameplay.worldItems().all().size() == 1 &&
                gameplay.inventory().at(0).has_value() && gameplay.inventory().at(0)->count == 9,
                "World item must remain while player is outside pickup range");

        // Return to the original drop, then collect the item on the next fixed tick.
        gameplay.player().setPosition(droppedPosition);
        gameplay.advance(1.0f / 60.0f);
        require(gameplay.worldItems().all().empty() &&
                gameplay.inventory().at(0).has_value() && gameplay.inventory().at(0)->count == 10,
                "World pickup must restore dropped item to original stack");

        bool seenPickup = false;
        for (const auto& event : gameplay.events().take())
            seenPickup |= event.type == ve::GameplayEventType::ItemPickedUp;
        require(seenPickup, "Pickup must publish event");

        std::cout << "[PASS] Session drop / pickup events, pickup delay, range and rollback\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
