#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include "render/shadow/ShadowMapping.h"
#include "render/shadow/ShadowFrameAdapter.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        const ve::Vec3 startupCamera{ 8.5f, 21.5f, 27.5f };
        ve::ShadowSettings software{};
        software.resolution = 384;
        software.pcfRadius = 0;
        software.normalOffset = 0.05f;

        const ve::ShadowFrame firstFrame = ve::ShadowMapping::prepare({}, startupCamera, software);
        require(firstFrame.lights.empty(), "No shadow casters must produce an empty frame");
        require(firstFrame.settings.resolution == 384, "Shadow settings corrupted");

        ve::RenderFrame frame{};
        frame.cameraPosition = startupCamera;
        const ve::ShadowFrame adapterFrame = ve::ShadowFrameAdapter::build(frame, software);
        require(adapterFrame.lights.empty(), "Adapter should not create unrequested shadows");
        std::cout << "[PASS] Software startup ShadowSettings and RenderFrame adapter\n";

        ve::ShadowSettings invalid = software;
        invalid.resolution = 3000;
        bool rejected = false;
        try {
            (void)ve::ShadowMapping::prepare({}, startupCamera, invalid);
        } catch (const std::invalid_argument& error) {
            rejected = std::string(error.what()).find("resolution") != std::string::npos;
        }
        require(rejected, "Invalid settings must report the failing field");

        invalid = software;
        invalid.normalOffset = -0.1f;
        rejected = false;
        try {
            (void)ve::ShadowMapping::prepare({}, startupCamera, invalid);
        } catch (const std::invalid_argument& error) {
            rejected = std::string(error.what()).find("normalOffset") != std::string::npos;
        }
        require(rejected, "Invalid receiver offset must report the failing field");

        const ve::Vec3 invalidCamera{ std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f };
        rejected = false;
        try {
            (void)ve::ShadowMapping::prepare({}, invalidCamera, software);
        } catch (const std::invalid_argument& error) {
            rejected = std::string(error.what()).find("cameraPosition") != std::string::npos;
        }
        require(rejected, "Invalid camera must report the failing field");
        std::cout << "[PASS] Invalid settings identify resolution, normalOffset and cameraPosition\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
