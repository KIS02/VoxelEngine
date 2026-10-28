#include <cmath>
#include <iostream>
#include <stdexcept>

#include "render/color/ToneMapping.h"
#include "render/color/ColorSpace.h"


namespace
{
    void require( bool valid, const char* message ) {
        if (!valid)
            throw std::runtime_error(message);
    }

    bool close( float a, float b ) { return std::fabs(a - b) < 0.0001f; }
}


int main() {
    try {
        const ve::ToneMappingSettings settings{ true, 1.0f, ve::ToneMappingOperator::Reinhard };
        const ve::LinearRGB mapped = ve::toneMapLinear(ve::LinearRGB{ 4.0f, 1.0f, 0.0f }, settings);
        require(close(mapped.r, 0.8f) && close(mapped.g, 0.5f) && close(mapped.b, 0.0f),
                "Reinhard mapping is incorrect");
        require(ve::toneMapToSrgb(ve::LinearRGB{ 4.0f, 1.0f, 0.0f }, 73, settings) != 0,
                "Tone Mapping produced empty pixel");
        require(ve::getAlpha(ve::toneMapToSrgb(ve::LinearRGB{ 4.0f, 1.0f, 0.0f }, 73, settings)) == 73,
                "Tone Mapping must preserve alpha");

        const ve::ToneMappingSettings bright{ true, 2.0f, ve::ToneMappingOperator::Reinhard };
        require(close(ve::toneMapLinear(ve::LinearRGB{ 1.0f, 0.0f, 0.0f }, bright).r, 2.0f/3.0f),
                "Exposure handling is incorrect");

        const ve::ToneMappingSettings clamp{ true, 1.0f, ve::ToneMappingOperator::LinearClamp };
        require(close(ve::toneMapLinear(ve::LinearRGB{ 4.0f, 0.0f, 0.0f }, clamp).r, 1.0f),
                "Linear clamp reference is incorrect");

        bool rejected = false;
        try {
            ve::toneMapLinear(ve::LinearRGB{ 1.0f, 1.0f, 1.0f }, ve::ToneMappingSettings{ true, 0.0f });
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "Zero exposure must fail");
        std::cout << "[PASS] HDR > 1 is preserved until Tone Mapping\n";
        std::cout << "[PASS] Reinhard, exposure, alpha and invalid inputs\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
