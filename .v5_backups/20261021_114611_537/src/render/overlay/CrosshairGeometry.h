#pragma once

#include <array>



namespace ve
{
    struct ScreenRect
    {
        int x = 0;
        int y = 0;

        int width = 0;
        int height = 0;
    };



    class CrosshairGeometry {
        public:
            static std::array<ScreenRect, 2> create(
                int framebufferWidth, int framebufferHeight,
                int armLength = 7, int thickness = 3
            );
    };
}