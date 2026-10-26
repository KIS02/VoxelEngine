#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "render/color/ColorSpace.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    bool near( float actual, float expected, float tolerance = 1.0e-5f ) {
        return std::fabs(actual - expected) <= tolerance;
    }


    void testSrgbTransferFunction() {
        require(near(ve::srgbToLinear(0.0f), 0.0f), "sRGB black must decode to zero");
        require(near(ve::srgbToLinear(1.0f), 1.0f), "sRGB white must decode to one");
        require(near(ve::srgbToLinear(0.04045f), 0.0031308f), "sRGB decoding breakpoint mismatch");
        require(near(ve::linearToSrgb(0.0031308f), 0.04045f), "sRGB encoding breakpoint mismatch");
        require(near(ve::srgbToLinear(128.0f / 255.0f), 0.2158605f), "Mid-gray decoding mismatch");

        std::cout << "[PASS] Exact sRGB transfer function\n";
    }


    void testRoundTripAndAlpha() {
        for (int channel = 0; channel < 256; ++channel) {
            const ve::Color32 source = ve::makeColor(
                static_cast<std::uint8_t>(channel),
                static_cast<std::uint8_t>(255 - channel),
                static_cast<std::uint8_t>(channel),
                73
            );

            const ve::LinearRGB linear = ve::decodeSrgb(source);
            const ve::Color32 restored = ve::encodeSrgb(linear, ve::getAlpha(source));

            require(restored == source, "Color32 sRGB round trip or alpha preservation failed");
        }

        std::cout << "[PASS] All 256 channel values round-trip and preserve alpha\n";
    }


    void testLinearArithmetic() {
        const ve::LinearRGB base = ve::decodeSrgb(ve::makeColor(128, 128, 128));
        const ve::LinearRGB shaded = base * ve::LinearRGB{ 0.5f, 0.5f, 0.5f };
        const ve::Color32 result = ve::encodeSrgb(shaded, 17);

        require(ve::getRed(result) == 92, "Linear half-intensity sRGB value must be 92");
        require(ve::getGreen(result) == 92, "Linear green channel must match");
        require(ve::getBlue(result) == 92, "Linear blue channel must match");
        require(ve::getAlpha(result) == 17, "Alpha must not undergo gamma correction");

        const ve::LinearRGB combined = ve::LinearRGB{ 0.2f, 0.3f, 0.4f } +
                                       ve::LinearRGB{ 0.1f, 0.2f, 0.3f };

        require(near(combined.r, 0.3f) && near(combined.g, 0.5f) && near(combined.b, 0.7f),
                "Linear RGB addition failed");

        std::cout << "[PASS] Linear-space RGB operations and alpha\n";
    }


    void testOutputClamping() {
        const ve::Color32 result = ve::encodeSrgb(ve::LinearRGB{ -3.0f, 0.5f, 2.0f }, 91);

        require(ve::getRed(result) == 0, "Negative linear red must clamp to zero");
        require(ve::getGreen(result) == 188, "Linear 0.5 must encode to sRGB 188");
        require(ve::getBlue(result) == 255, "Linear HDR blue must clamp to 255");
        require(ve::getAlpha(result) == 91, "Output alpha must remain unchanged");

        std::cout << "[PASS] Output clamping at encoded boundary\n";
    }


    void testInvalidInput() {
        bool rejected = false;

        try {
            (void)ve::srgbToLinear(1.1f);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Out-of-range sRGB input must be rejected");

        rejected = false;

        try {
            (void)ve::encodeSrgb(ve::LinearRGB{
                std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f
            });
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "NaN linear RGB must be rejected");

        std::cout << "[PASS] Invalid color inputs rejected\n";
    }
}


int main() {
    try {
        testSrgbTransferFunction();
        testRoundTripAndAlpha();
        testLinearArithmetic();
        testOutputClamping();
        testInvalidInput();

        std::cout << "All LinearColorSpace tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}