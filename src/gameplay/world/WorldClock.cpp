#include "gameplay/world/WorldClock.h"

#include <cmath>
#include <limits>
#include <stdexcept>


namespace ve
{
    namespace
    {
        void validateDayLength( double seconds ) {
            if (!std::isfinite(seconds) || seconds < 60.0 || seconds > 7200.0)
                throw std::invalid_argument("World day duration must be between 60 and 7200 seconds");
        }
    }


    void WorldClock::reset() {
        day_ = 0;
        dayLengthSeconds_ = DefaultDaySeconds;
        secondsOfDay_ = dayLengthSeconds_ * 0.5;
    }

    void WorldClock::advance( double fixedDeltaSeconds ) {
        if (!std::isfinite(fixedDeltaSeconds) || fixedDeltaSeconds < 0.0)
            throw std::invalid_argument("Invalid WorldClock delta time");

        const double elapsed = secondsOfDay_ + fixedDeltaSeconds;
        const double daysPassed = std::floor(elapsed / dayLengthSeconds_);
        // Check the floating-point boundary before converting to uint64_t.
        // 2^64 is representable as double, but cannot be represented by uint64_t.
        if (daysPassed >= 18446744073709551616.0)
            throw std::overflow_error("WorldClock day counter overflow");

        const auto wholeDays = static_cast<std::uint64_t>(daysPassed);
        if (wholeDays > std::numeric_limits<std::uint64_t>::max() - day_)
            throw std::overflow_error("WorldClock day counter overflow");

        day_ += wholeDays;
        secondsOfDay_ = std::fmod(elapsed, dayLengthSeconds_);
        if (secondsOfDay_ < 0.0)
            secondsOfDay_ += dayLengthSeconds_;
    }

    void WorldClock::restore( std::uint64_t day, double secondsOfDay, double dayLengthSeconds ) {
        validateDayLength(dayLengthSeconds);
        if (!std::isfinite(secondsOfDay) || secondsOfDay < 0.0 || secondsOfDay >= dayLengthSeconds)
            throw std::invalid_argument("Invalid WorldClock saved time");

        day_ = day;
        secondsOfDay_ = secondsOfDay;
        dayLengthSeconds_ = dayLengthSeconds;
    }

    void WorldClock::setDayLengthSeconds( double seconds ) {
        validateDayLength(seconds);
        secondsOfDay_ = normalizedDay() * seconds;
        dayLengthSeconds_ = seconds;
    }

    void WorldClock::setTimeOfDayHours( double hours ) {
        if (!std::isfinite(hours) || hours < 0.0 || hours >= 24.0)
            throw std::invalid_argument("World time of day must be within [0, 24)");

        secondsOfDay_ = hours / 24.0 * dayLengthSeconds_;
    }
}
