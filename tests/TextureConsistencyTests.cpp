#include <exception>
#include <iostream>
#include <stdexcept>

#include "math/Vec2.h"

#include "render/Color.h"
#include "assets/texture/Texture2D.h"

#include "render/texture/UVTestTexture.h"



namespace
{
    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    void testDimensions() {
        const ve::Texture2D texture = ve::createUVTestTexture();

        require(texture.width() == 64, "Texture width must be 64");
        require(texture.height() == 64, "Texture height must be 64");

        std::cout << "[PASS] Shared Texture dimensions\n";
    }



    void testCornerPixels() {
        const ve::Texture2D texture = ve::createUVTestTexture();

        require(texture.pixel(0, 0) == ve::makeColor(255, 0, 0),
                "Bottom-left pixel must be red");

        require(texture.pixel(63, 0) == ve::makeColor(0, 255, 0),
                "Bottom-right pixel must be green");

        require(texture.pixel(0, 63) == ve::makeColor(0, 0, 255),
                "Top-left pixel must be blue");

        require(texture.pixel(63, 63) == ve::makeColor(255, 255, 0),
                "Top-right pixel must be yellow");

        std::cout << "[PASS] Shared Texture corner pixels\n";
    }



    void testUVSampling() {
        const ve::Texture2D texture = ve::createUVTestTexture();

        require(texture.sampleNearest(ve::Vec2{ 0.1f, 0.1f }) == ve::makeColor(255, 0, 0),
                "Bottom-left UV must sample red");

        require(texture.sampleNearest(ve::Vec2{ 0.9f, 0.1f }) == ve::makeColor(0, 255, 0),
                "Bottom-right UV must sample green");

        require(texture.sampleNearest(ve::Vec2{ 0.1f, 0.9f }) == ve::makeColor(0, 0, 255),
                "Top-left UV must sample blue");

        require(texture.sampleNearest(ve::Vec2{ 0.9f, 0.9f }) == ve::makeColor(255, 255, 0),
                "Top-right UV must sample yellow");

        std::cout << "[PASS] Shared Texture UV sampling\n";
    }
}



int main() {
    try {
        testDimensions();
        testCornerPixels();
        testUVSampling();

        std::cout << "\nAll texture consistency tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}