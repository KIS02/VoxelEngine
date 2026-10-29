#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "gameplay/world/WorldClock.h"
#include "ui/UIFrameData.h"


namespace ve
{
    class WorldTimeHud {
        public:
            static void append( UIFrameData& ui, const WorldClock& clock ) {
                const int totalMinutes = static_cast<int>(std::floor(clock.hours() * 60.0));
                const int hour = totalMinutes / 60;
                const int minute = totalMinutes % 60;

                const char* period = "NIGHT";
                if (hour >= 6 && hour < 12)
                    period = "MORNING";
                else if (hour >= 12 && hour < 18)
                    period = "AFTERNOON";
                else if (hour >= 18 && hour < 20)
                    period = "EVENING";

                ui.hotbarOverlay.enabled = true;
                ui.hotbarOverlay.lines.push_back("DAY " + std::to_string(clock.day() + 1) +
                    " | " + (hour < 10 ? "0" : "") + std::to_string(hour) + ":" +
                    (minute < 10 ? "0" : "") + std::to_string(minute) + " " + period);
            }
    };
}
