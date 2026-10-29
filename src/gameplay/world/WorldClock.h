#pragma once

#include <cstdint>


namespace ve
{
    class WorldClock {
        private:
            static constexpr double DefaultDaySeconds = 1200.0;
            std::uint64_t day_ = 0;
            double secondsOfDay_ = DefaultDaySeconds * 0.5;
            double dayLengthSeconds_ = DefaultDaySeconds;


        public:
            void reset();
            void advance( double fixedDeltaSeconds );
            void restore( std::uint64_t day, double secondsOfDay, double dayLengthSeconds );
            void setDayLengthSeconds( double seconds );
            void setTimeOfDayHours( double hours );

            std::uint64_t day() const { return day_; }
            double secondsOfDay() const { return secondsOfDay_; }
            double dayLengthSeconds() const { return dayLengthSeconds_; }
            double hours() const { return 24.0 * secondsOfDay_ / dayLengthSeconds_; }
            double normalizedDay() const { return secondsOfDay_ / dayLengthSeconds_; }
    };
}
