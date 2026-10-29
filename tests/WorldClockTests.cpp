#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "gameplay/world/WorldClock.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    bool close( double a, double b ) { return std::fabs(a - b) < 1.0e-6; }
}


int main() {
    try {
        ve::WorldClock clock;
        require(clock.day() == 0 && close(clock.hours(), 12.0), "Initial time must be noon on day zero");
        for (int i = 0; i < 60; ++i)
            clock.advance(1.0 / 60.0);
        require(close(clock.secondsOfDay(), 601.0), "Sixty fixed ticks must advance one second");
        clock.setTimeOfDayHours(23.0);
        clock.advance(50.0);
        require(clock.day() == 1 && close(clock.hours(), 0.0), "Midnight rollover failed");
        clock.advance(2405.0);
        require(clock.day() == 3 && close(clock.secondsOfDay(), 5.0), "Multi-day rollover failed");
        clock.setDayLengthSeconds(600.0);
        require(close(clock.hours(), 0.1), "Changing day duration must preserve time of day");
        clock.restore(25, 150.0, 600.0);
        require(clock.day() == 25 && close(clock.hours(), 6.0), "Restored world time invalid");
        clock.reset();
        require(clock.day() == 0 && close(clock.hours(), 12.0), "World time reset failed");

        bool rejected = false;
        try { clock.advance(std::numeric_limits<double>::quiet_NaN()); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "NaN delta must be rejected");

        rejected = false;
        try {
            clock.restore(std::numeric_limits<std::uint64_t>::max(), 0.0, 60.0);
            clock.advance(60.0);
        } catch (const std::overflow_error&) {
            rejected = true;
        }
        require(rejected, "WorldClock must reject uint64_t day overflow");
        require(clock.day() == std::numeric_limits<std::uint64_t>::max(),
                "Overflow must not change the day counter");

        clock.reset();
        clock.setDayLengthSeconds(60.0);
        rejected = false;
        try {
            clock.advance(60.0 * 18446744073709551616.0);
        } catch (const std::overflow_error&) {
            rejected = true;
        }
        require(rejected && clock.day() == 0, "WorldClock must reject 2^64 day rollover");

        rejected = false;
        try { clock.restore(4, 600.0, 600.0); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "Out-of-range saved time must be rejected");

        std::cout << "[PASS] WorldClock fixed ticks, midnight, time scale, reset, validation\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
