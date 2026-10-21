#include <exception>
#include <iostream>
#include <stdexcept>

#include "render/overlay/CrosshairGeometry.h"



namespace
{
    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    void requireRect( const ve::ScreenRect& rect, int x, int y, int width, int height ) {
        require(
            rect.x == x && rect.y == y &&
            rect.width == width && rect.height == height,
            "Crosshair rectangle does not match expected coordinates"
        );
    }



    void testDefaultGeometry() {
        const auto rectangles = ve::CrosshairGeometry::create(800, 600);

        requireRect(rectangles[0], 393, 299, 15, 3);
        requireRect(rectangles[1], 399, 293, 3, 15);

        std::cout << "[PASS] Default 800x600 Crosshair geometry\n";
    }



    void testOddFramebufferDimensions() {
        const auto rectangles = ve::CrosshairGeometry::create(801, 601);

        requireRect(rectangles[0], 393, 299, 15, 3);
        requireRect(rectangles[1], 399, 293, 3, 15);

        std::cout << "[PASS] Odd framebuffer dimensions\n";
    }



    void testCustomDimensions() {
        const auto rectangles = ve::CrosshairGeometry::create(800, 600, 3, 2);

        requireRect(rectangles[0], 397, 299, 7, 2);
        requireRect(rectangles[1], 399, 297, 2, 7);

        std::cout << "[PASS] Custom Crosshair size and thickness\n";
    }



    void testSmallFramebufferClipping() {
        const auto rectangles = ve::CrosshairGeometry::create(3, 3);

        requireRect(rectangles[0], 0, 0, 3, 3);
        requireRect(rectangles[1], 0, 0, 3, 3);

        std::cout << "[PASS] Crosshair clipping in small framebuffer\n";
    }



    void testEmptyFramebuffer() {
        const auto rectangles = ve::CrosshairGeometry::create(0, 600);

        requireRect(rectangles[0], 0, 0, 0, 0);
        requireRect(rectangles[1], 0, 0, 0, 0);

        std::cout << "[PASS] Empty framebuffer handling\n";
    }



    void testInvalidDimensions() {
        int rejected = 0;

        const auto expectInvalid = [&]( int width, int height, int length, int thickness ) {
            try {
                (void)ve::CrosshairGeometry::create(width, height, length, thickness);
            }
            catch (const std::invalid_argument&) {
                ++rejected;
            }
        };



        expectInvalid(-1, 600, 7, 3);
        expectInvalid(800, 600, -1, 3);
        expectInvalid(800, 600, 7, 0);

        require(rejected == 3, "All invalid Crosshair inputs must be rejected");

        std::cout << "[PASS] Invalid Crosshair dimensions\n";
    }
}



int main() {
    try {
        testDefaultGeometry();
        testOddFramebufferDimensions();
        testCustomDimensions();
        testSmallFramebufferClipping();
        testEmptyFramebuffer();
        testInvalidDimensions();

        std::cout << "\nAll Crosshair Geometry tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}