#include <cmath>
#include <iostream>
#include <stdexcept>

#include "gameplay/world/SunCycle.h"
#include "scene/lighting/SceneLightCollector.h"
#include "render/shadow/ShadowFrameFingerprint.h"
#include "ui/WorldTimeHud.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    void updateSun( ve::SceneObject& sun, const ve::SunCycleState& state ) {
        sun.rotationDegrees = state.rotationDegrees;
        sun.light->color = state.color;
        sun.light->intensity = state.intensity;
        sun.light->ambient = state.ambient;
        sun.light->diffuse = state.diffuse;
        sun.light->castsShadows = state.castsShadows;
    }
}


int main() {
    try {
        ve::WorldClock clock;
        ve::SceneObjectRegistry registry;
        ve::SceneObject sun{};
        sun.light = ve::LightComponent{};
        sun.light->type = ve::LightType::Directional;
        const auto id = registry.create(sun);
        ve::SceneObject* object = registry.find(id);
        require(object != nullptr, "Scene sun registration failed");

        clock.setTimeOfDayHours(12.0);
        const auto noon = ve::SunCycle::sample(clock, true);
        require(noon.daylight && noon.castsShadows && noon.intensity > 0.9f, "Noon must light scene");
        updateSun(*object, noon);
        ve::LightingSettings noonLights{};
        ve::SceneLightCollector::collect(registry, noonLights);
        require(noonLights.sceneLights.size() == 1 &&
                noonLights.sceneLights[0].directionToLight.y > 0.99f &&
                noonLights.sceneLights[0].castsShadows, "Noon RenderFrame light snapshot invalid");

        ve::RenderFrame noonFrame{};
        noonFrame.lighting = noonLights;
        const ve::ShadowSettings shadowSettings{};
        const auto noonFingerprint = ve::ShadowFrameFingerprint::calculate(noonFrame, shadowSettings);

        clock.setTimeOfDayHours(0.0);
        const auto midnight = ve::SunCycle::sample(clock, true);
        require(!midnight.daylight && !midnight.castsShadows &&
                midnight.diffuse == 0.0f && midnight.intensity < noon.intensity,
                "Night must dim sunlight and disable sun shadows");
        updateSun(*object, midnight);
        ve::LightingSettings midnightLights{};
        ve::SceneLightCollector::collect(registry, midnightLights);
        require(midnightLights.sceneLights.size() == 1 &&
                midnightLights.sceneLights[0].directionToLight.y < -0.99f &&
                !midnightLights.sceneLights[0].castsShadows,
                "Night SceneLightCollector snapshot invalid");

        ve::RenderFrame midnightFrame{};
        midnightFrame.lighting = midnightLights;
        const auto nightFingerprint = ve::ShadowFrameFingerprint::calculate(midnightFrame, shadowSettings);
        require(noonFingerprint != nightFingerprint,
                "Sun rotation and shadow toggle must invalidate Shadow depth cache");

        clock.setTimeOfDayHours(7.0);
        const auto morning = ve::SunCycle::sample(clock, true);
        require(morning.daylight && morning.intensity > midnight.intensity &&
                morning.intensity < noon.intensity, "Sunrise transition should be smooth");
        require(!ve::SunCycle::sample(clock, false).castsShadows,
                "Shadow preview toggle must remain authoritative");

        ve::UIFrameData hud{};
        ve::WorldTimeHud::append(hud, clock);
        require(hud.hotbarOverlay.enabled && hud.hotbarOverlay.lines.size() == 1 &&
                hud.hotbarOverlay.lines[0].find("07:00 MORNING") != std::string::npos,
                "World time HUD must show current game hour");

        std::cout << "[PASS] SunCycle noon/night/twilight, SceneLightCollector and WorldTimeHud\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
