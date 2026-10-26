#include <exception>
#include <iostream>
#include <stdexcept>

#include "render/backends/opengl/resources/OpenGLFrameResources.h"
#include "render/common/MaterialCachePolicy.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void testActiveResources() {
        ve::OpenGLFrameResources resources;
        resources.track(101, 201);
        resources.track(101, 201);
        resources.track(102, 201);

        require(resources.meshIds().size() == 2, "Active mesh IDs must deduplicate");
        require(resources.textureIds().size() == 1, "Shared textures must remain a single cache entry");
        require(!ve::MaterialCachePolicy::shouldRelease(201, resources.textureIds()),
                "Active shared texture must not be released");
        require(resources.meshIds().contains(101) && resources.meshIds().contains(102),
                "All drawn meshes must remain resident");
        require(ve::MaterialCachePolicy::shouldRelease(999, resources.meshIds()),
                "Inactive mesh must be eligible for release");

        std::cout << "[PASS] Active GPU resource retention and deduplication\n";
    }


    void testInactiveResources() {
        const ve::OpenGLFrameResources emptyFrame;
        require(ve::MaterialCachePolicy::shouldRelease(101, emptyFrame.meshIds()),
                "An empty frame must release stale meshes");
        require(ve::MaterialCachePolicy::shouldRelease(201, emptyFrame.textureIds()),
                "An empty frame must release stale textures");

        bool rejected = false;
        ve::OpenGLFrameResources frame;
        try {
            frame.track(0, 201);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "Zero mesh IDs are invalid for the GPU cache");
        require(frame.meshIds().empty() && frame.textureIds().empty(),
                "Failed tracking must leave the active sets unchanged");

        std::cout << "[PASS] Empty frames, invalid IDs and stale resource release\n";
    }
}


int main() {
    try {
        testActiveResources();
        testInactiveResources();
        std::cout << "All OpenGLFrameResources tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
