#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <unordered_set>

#include "render/MeshIdGenerator.h"
#include "render/api/RenderFrame.h"



namespace
{
    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    void testSequentialIds() {
        ve::MeshIdGenerator generator{};


        require(generator.generate() == 1, "First Mesh ID must be 1");
        require(generator.generate() == 2, "Second Mesh ID must be 2");
        require(generator.generate() == 3, "Third Mesh ID must be 3");


        std::cout << "[PASS] Sequential Mesh ID generation\n";
    }



    void testUniqueIds() {
        ve::MeshIdGenerator generator{};

        std::unordered_set<std::uint64_t> ids;



        for (int i = 0; i < 10000; ++i) {
            const std::uint64_t id = generator.generate();

            require(id != 0, "Generated Mesh ID must not be zero");
            require(ids.insert(id).second, "Duplicate Mesh ID generated");
        }


        require(ids.size() == 10000, "Mesh ID count is incorrect");


        std::cout << "[PASS] 10000 unique Mesh IDs\n";
    }



    void testRevisionIndependence() {
        ve::MeshIdGenerator generator{};

        const std::uint64_t meshId = generator.generate();



        ve::RenderDraw draw{};

        draw.meshId = meshId;
        draw.meshRevision = 1;



        // Simulate a Mesh Rebuild

        ++draw.meshRevision;



        require(draw.meshId == meshId, "Mesh ID must remain stable");
        require(draw.meshRevision == 2, "Revision must increase");


        std::cout << "[PASS] Mesh ID remains stable across revisions\n";
    }



    void testRenderFrameIdentity() {
        ve::MeshIdGenerator generator{};

        const std::uint64_t firstId = generator.generate();
        const std::uint64_t secondId = generator.generate();



        ve::RenderFrame frame{};

        frame.draws.push_back(ve::RenderDraw{
            nullptr,
            ve::Mat4::identity(),
            1,
            firstId
        });

        frame.draws.push_back(ve::RenderDraw{
            nullptr,
            ve::Mat4::identity(),
            3,
            secondId
        });



        require(frame.draws.size() == 2, "RenderFrame must contain two draws");

        require(frame.draws[0].meshId != frame.draws[1].meshId, "Different Meshes need different IDs");

        require(frame.draws[0].meshRevision == 1, "First Mesh revision is incorrect");
        require(frame.draws[1].meshRevision == 3, "Second Mesh revision is incorrect");


        std::cout << "[PASS] RenderFrame preserves Mesh identity and revision\n";
    }
}



int main() {
    try {
        testSequentialIds();
        testUniqueIds();
        testRevisionIndependence();
        testRenderFrameIdentity();


        std::cout << "\nAll Mesh ID tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}