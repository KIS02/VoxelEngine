#include "render/overlay/CrosshairGeometry.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>



namespace ve
{
    std::array<ScreenRect, 2> CrosshairGeometry::create(
        int framebufferWidth, int framebufferHeight,
        int armLength, int thickness
    ) {
        if (framebufferWidth < 0 || framebufferHeight < 0) {
            throw std::invalid_argument("Framebuffer dimensions must be nonnegative");
        }

        if (armLength < 0 || thickness <= 0) {
            throw std::invalid_argument("Invalid Crosshair dimensions");
        }



        if (framebufferWidth == 0 || framebufferHeight == 0) {
            return {};
        }



        // Screen Coordinates Use a Top-Left Origin

        const std::int64_t centerX = framebufferWidth / 2;
        const std::int64_t centerY = framebufferHeight / 2;

        const std::int64_t halfLength = armLength;
        const std::int64_t halfThickness = thickness / 2;
        const std::int64_t remainingThickness = (static_cast<std::int64_t>(thickness) + 1) / 2;



        // Clip Each Rectangle to the Framebuffer Bounds

        const auto clipRectangle = [&]( std::int64_t left, std::int64_t top,
                                        std::int64_t right, std::int64_t bottom ) {
            const std::int64_t x0 = std::clamp(left, 0LL, static_cast<std::int64_t>(framebufferWidth));
            const std::int64_t y0 = std::clamp(top, 0LL, static_cast<std::int64_t>(framebufferHeight));

            const std::int64_t x1 = std::clamp(right, 0LL, static_cast<std::int64_t>(framebufferWidth));
            const std::int64_t y1 = std::clamp(bottom, 0LL, static_cast<std::int64_t>(framebufferHeight));


            return ScreenRect{
                static_cast<int>(x0),
                static_cast<int>(y0),
                static_cast<int>(std::max<std::int64_t>(0, x1 - x0)),
                static_cast<int>(std::max<std::int64_t>(0, y1 - y0))
            };
        };



        // Horizontal Rectangle

        const ScreenRect horizontal = clipRectangle(
            centerX - halfLength,
            centerY - halfThickness,
            centerX + halfLength + 1,
            centerY + remainingThickness
        );



        // Vertical Rectangle

        const ScreenRect vertical = clipRectangle(
            centerX - halfThickness,
            centerY - halfLength,
            centerX + remainingThickness,
            centerY + halfLength + 1
        );



        return { horizontal, vertical };
    }
}