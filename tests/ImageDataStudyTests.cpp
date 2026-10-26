#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "assets/texture/ImageData.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    void testValidImage() {
        ve::ImageData image{};
        image.width = 2;
        image.height = 2;
        image.colorSpace = ve::TextureColorSpace::SRGB;
        image.rgba8 = {
            255, 0, 0, 73,      0, 255, 0, 255,
            0, 0, 255, 255,     255, 255, 0, 255
        };

        image.validate();

        const std::size_t pixelIndex = 1;
        const std::size_t offset = pixelIndex * 4;
        require(image.rgba8[offset] == 0, "Bottom-right red channel must be zero");
        require(image.rgba8[offset + 1] == 255, "Bottom-right green channel must be 255");
        require(image.rgba8[3] == 73, "Bottom-left alpha must remain 73");
        require(image.colorSpace == ve::TextureColorSpace::SRGB, "Color space was lost");

        std::cout << "[PASS] 2x2 RGBA8 layout and alpha\n";
    }

    void testInvalidByteCount() {
        ve::ImageData image{};
        image.width = 2;
        image.height = 2;
        image.rgba8.resize(15);

        bool rejected = false;
        try {
            image.validate();
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Invalid RGBA8 byte count must be rejected");
        std::cout << "[PASS] Invalid RGBA8 length rejected\n";
    }

    void testInvalidDimensions() {
        ve::ImageData image{};
        image.width = 0;
        image.height = 1;

        bool rejected = false;
        try {
            image.validate();
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Zero image width must be rejected");
        std::cout << "[PASS] Invalid dimensions rejected\n";
    }
}


int main() {
    try {
        testValidImage();
        testInvalidByteCount();
        testInvalidDimensions();
        std::cout << "All ImageData study tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}