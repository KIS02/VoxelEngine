#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <type_traits>

#include "render/shadow/ShadowMapping.h"


namespace
{
    static_assert(sizeof(int) == 4 && sizeof(float) == 4,
                  "ShadowSettings layout test assumes 32-bit int and float");
    static_assert(std::is_standard_layout_v<ve::ShadowSettings>,
                  "ShadowSettings must remain standard layout");
    static_assert(sizeof(ve::ShadowSettings) == 32,
                  "ShadowSettings ABI mismatch: expected normalOffset and pcfRadius");
    static_assert(offsetof(ve::ShadowSettings, normalOffset) == 24,
                  "ShadowSettings ABI mismatch: normalOffset offset changed");
    static_assert(offsetof(ve::ShadowSettings, pcfRadius) == 28,
                  "ShadowSettings ABI mismatch: pcfRadius offset changed");


    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        ve::ShadowSettings settings{};
        settings.resolution = 192;
        settings.directionalHalfExtent = 12.0f;
        settings.normalOffset = 0.05f;
        settings.pcfRadius = 0;

        const ve::ShadowFrame result = ve::ShadowMapping::prepare(
            {}, ve::Vec3{ 0.0f, 0.0f, 0.0f }, settings
        );

        require(result.settings.resolution == settings.resolution, "Shadow resolution changed");
        require(std::fabs(result.settings.normalOffset - settings.normalOffset) < 1.0e-6f,
                "Shadow normal offset corrupted across translation units");
        require(result.settings.pcfRadius == settings.pcfRadius,
                "Shadow PCF radius corrupted across translation units");
        require(result.lights.empty(), "Empty shadow scene unexpectedly created light maps");

        std::cout << "[PASS] ShadowSettings ABI: sizeof=32, normalOffset=24, pcfRadius=28\n";
        std::cout << "[PASS] ShadowSettings passes intact through ShadowMapping::prepare\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
