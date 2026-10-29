#include <cmath>
#include <iostream>
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
        ve::GameplaySession session;
        session.start(ve::Vec3{ 8.5f, 21.5f, 27.5f });
        session.events().take();
        session.worldClock().setDayLengthSeconds(60.0);
        session.worldClock().setTimeOfDayHours(23.999);
        const double before = session.worldClock().secondsOfDay();
        require(session.advance(0.001f).steps == 0, "Tiny frame should not advance the world clock");
        require(session.worldClock().secondsOfDay() == before, "World clock ran outside Fixed Tick");
        require(session.advance(1.0f / 60.0f).steps >= 1, "Fixed Tick should advance world time");
        require(session.worldClock().day() == 1, "World day rollover event missing");
        const auto events = session.events().take();
        require(events.size() == 1 && events[0].type == ve::GameplayEventType::WorldDayChanged,
                "New day event not delivered");
        session.respawn();
        require(session.worldClock().day() == 1, "Respawn must not restart the world clock");
        session.start(ve::Vec3{ 8.5f, 21.5f, 27.5f });
        require(session.worldClock().day() == 0 && std::fabs(session.worldClock().hours() - 12.0) < 1.0e-8,
                "New GameplaySession start must reset world time");
        std::cout << "[PASS] GameplaySession WorldClock Fixed Tick, events, respawn and restart\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
