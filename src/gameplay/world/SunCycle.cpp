#include "gameplay/world/SunCycle.h"

#include <algorithm>
#include <cmath>


namespace ve
{
    SunCycleState SunCycle::sample( const WorldClock& clock, bool shadowsEnabled ) {
        constexpr double Pi = 3.14159265358979323846;
        const double angle = (clock.hours() - 6.0) * Pi / 12.0;
        const float elevation = static_cast<float>(std::sin(angle));
        const float daylight = std::clamp(elevation, 0.0f, 1.0f);
        const float smoothDaylight = daylight * daylight * (3.0f - 2.0f * daylight);
        const float dusk = 1.0f - smoothDaylight;

        SunCycleState state{};
        // SceneLightCollector rotates the local (0, 0, -1) emission direction.
        // A negative X angle makes positive-Y directionToLight at midday.
        state.rotationDegrees = Vec3{ static_cast<float>(-angle * 180.0 / Pi), -48.0f, 0.0f };
        state.intensity = 0.14f + 0.86f * smoothDaylight;
        state.ambient = 0.25f;
        state.diffuse = 0.75f * smoothDaylight;
        state.color = LinearRGB{
            0.55f + 0.45f * smoothDaylight,
            0.62f + 0.31f * smoothDaylight,
            1.00f - 0.20f * smoothDaylight
        };
        // Add warmer colors at sunrise and sunset, not during midnight.
        const float twilight = dusk * daylight;
        state.color.g -= 0.20f * twilight;
        state.color.b -= 0.36f * twilight;
        state.daylight = daylight > 0.0f;
        state.castsShadows = shadowsEnabled && daylight > 0.05f;
        return state;
    }
}
