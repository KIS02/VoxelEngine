
#include <iostream>
#include <stdexcept>

#include "render/overlay/SoftwareDebugOverlay.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void testDisabledOverlay() {
        ve::FrameBuffer buffer(320, 240);
        const ve::Color32 original = ve::makeColor(90, 120, 160);
        buffer.clear(original);

        ve::DebugOverlaySettings settings{};
        settings.lines = { "F3", "FPS: 60" };

        ve::SoftwareDebugOverlay::draw(buffer, settings);

        require(buffer.pixel(20, 20) == original, "Disabled overlay changed pixels");

        std::cout << "[PASS] Disabled overlay does not modify framebuffer\n";
    }


    void testBackgroundAndText() {
        ve::FrameBuffer buffer(320, 240);
        const ve::Color32 original = ve::makeColor(100, 150, 200);
        buffer.clear(original);

        ve::DebugOverlaySettings settings{};
        settings.enabled = true;
        settings.lines = { "F3", "XYZ: -1.50 / 2.00" };

        ve::SoftwareDebugOverlay::draw(buffer, settings);

        require(buffer.pixel(12, 12) == ve::makeColor(40, 60, 80), "Panel must darken scene");
        require(buffer.pixel(20, 20) == ve::makeColor(115, 220, 255), "Header glyph missing");
        require(buffer.pixel(20, 35) == ve::makeColor(255, 255, 255), "Body glyph missing");
        require(buffer.pixel(300, 200) == original, "Pixels outside panel must remain unchanged");

        std::cout << "[PASS] Translucent panel, glyphs and outside pixels\n";
    }


    void testClipping() {
        ve::FrameBuffer buffer(48, 64);
        buffer.clear(ve::makeColor(20, 20, 20));

        ve::DebugOverlaySettings settings{};
        settings.enabled = true;
        settings.lines = { "VERY LONG LINE", "SECOND LINE", "THIRD LINE" };

        ve::SoftwareDebugOverlay::draw(buffer, settings);

        require(buffer.pixel(12, 12) != ve::makeColor(20, 20, 20), "Clipped overlay missing");

        std::cout << "[PASS] Small framebuffer and long lines are safely clipped\n";
    }
}


int main() {
    try {
        testDisabledOverlay();
        testBackgroundAndText();
        testClipping();

        std::cout << "\nAll Software Debug Overlay tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
