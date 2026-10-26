#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "assets/texture/ImageData.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    void testValidPixels() {
        ve::ImageData image{};
        image.width = 2;
        image.height = 2;
        image.colorSpace = ve::TextureColorSpace::SRGB;
        image.rgba8 = {
            255, 0, 0, 73,     0, 255, 0, 255,
            0, 0, 255, 255,   255, 255, 0, 255
        };

        image.validate();

        const std::size_t offset = (0 * static_cast<std::size_t>(image.width) + 1) * 4;

        require(image.rgba8.size() == 16, "2x2 RGBA8 must contain 16 bytes");
        require(image.rgba8[offset] == 0, "Bottom-right red channel mismatch");
        require(image.rgba8[offset + 1] == 255, "Bottom-right green channel mismatch");
        require(image.rgba8[3] == 73, "Bottom-left alpha mismatch");
        require(image.colorSpace == ve::TextureColorSpace::SRGB, "Color space mismatch");

        std::cout << "[PASS] RGBA8 pixel memory layout\n";
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

        require(rejected, "Image with 15 bytes must be rejected");

        std::cout << "[PASS] Invalid byte count rejected\n";
    }

    void testInvalidDimensions() {
        ve::ImageData image{};
        image.width = 0;
        image.height = 2;

        bool rejected = false;

        try {
            image.validate();
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Image with zero width must be rejected");

        std::cout << "[PASS] Invalid dimensions rejected\n";
    }

    void testLinearColorSpaceMetadata() {
        ve::ImageData image{};
        image.width = 1;
        image.height = 1;
        image.colorSpace = ve::TextureColorSpace::Linear;
        image.rgba8 = { 128, 64, 32, 255 };

        image.validate();

        require(image.colorSpace == ve::TextureColorSpace::Linear, "Linear metadata mismatch");
        require(image.rgba8[0] == 128, "validate() must not transform pixel bytes");

        std::cout << "[PASS] Linear metadata preserves pixel bytes\n";
    }
}


int main() {
    try {
        testValidPixels();
        testInvalidByteCount();
        testInvalidDimensions();
        testLinearColorSpaceMetadata();

        std::cout << "All ImageData tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}