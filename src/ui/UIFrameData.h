#pragma once

#include <string>
#include <vector>

#include "math/Color.h"


namespace ve
{
    struct CrosshairSettings
    {
        bool enabled = false;

        Color32 color = makeColor(255, 255, 255);

        int armLength = 7;
        int thickness = 3;
    };


    struct DebugOverlaySettings
    {
        bool enabled = false;
        std::vector<std::string> lines{};
    };


    struct UIFrameData
    {
        CrosshairSettings crosshair{};
        DebugOverlaySettings debugOverlay{};
    };
}
