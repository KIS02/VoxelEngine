
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "render/MeshData.h"
#include "render/RenderFrame.h"
#include "render/pipeline/PipelineTriangleSelector.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    ve::MeshData createQuadMesh() {
        std::vector<ve::Vertex> vertices(4);

        vertices[0].position = ve::Vec3{ 0.0f, 0.0f, 0.0f };
        vertices[1].position = ve::Vec3{ 1.0f, 0.0f, 0.0f };
        vertices[2].position = ve::Vec3{ 1.0f, 1.0f, 0.0f };
        vertices[3].position = ve::Vec3{ 0.0f, 1.0f, 0.0f };

        vertices[0].uv = ve::Vec2{ 0.0f, 0.0f };
        vertices[1].uv = ve::Vec2{ 1.0f, 0.0f };
        vertices[2].uv = ve::Vec2{ 1.0f, 1.0f };
        vertices[3].uv = ve::Vec2{ 0.0f, 1.0f };

        vertices[0].color = ve::makeColor(255, 0, 0);
        vertices[1].color = ve::makeColor(0, 255, 0);
        vertices[2].color = ve::makeColor(0, 0, 255);
        vertices[3].color = ve::makeColor(255, 255, 0);

        std::vector<std::uint32_t> indices{
            0, 1, 2,
            0, 2, 3
        };

        return ve::MeshData(std::move(vertices), std::move(indices));
    }


    ve::RenderFrame createFrame( const ve::MeshData& mesh ) {
        ve::RenderFrame frame{};

        frame.view = ve::Mat4::translation(ve::Vec3{ 0.0f, 0.0f, -5.0f });
        frame.projection = ve::Mat4::identity();

        frame.draws.push_back(ve::RenderDraw{
            &mesh,
            ve::Mat4::translation(ve::Vec3{ 2.0f, 0.0f, 0.0f }),
            7,
            42
        });

        frame.draws.push_back(ve::RenderDraw{
            &mesh,
            ve::Mat4::translation(ve::Vec3{ 16.0f, 0.0f, 0.0f }),
            0,
            0
        });

        return frame;
    }


    void testInitialState() {
        ve::PipelineTriangleSelector selector;
        ve::RenderFrame frame{};

        require(!selector.hasSelection(), "Selector must initially be empty");
        require(!selector.snapshot().has_value(), "Snapshot must initially be empty");
        require(!selector.select(frame, 800, 600, 0, 0), "Empty frame must reject selection");

        std::cout << "[PASS] Empty selector and empty frame\n";
    }


    void testTriangleIndicesAndAttributes() {
        ve::MeshData mesh = createQuadMesh();
        ve::RenderFrame frame = createFrame(mesh);

        ve::PipelineTriangleSelector selector;

        require(selector.select(frame, 800, 600, 0, 0), "First triangle selection failed");

        const ve::PipelineTriangleSnapshot first = *selector.snapshot();

        require(first.identity.triangleIndex == 0, "Incorrect first triangle number");
        require(first.identity.vertexIndices[0] == 0, "Incorrect first vertex index");
        require(first.identity.vertexIndices[1] == 1, "Incorrect second vertex index");
        require(first.identity.vertexIndices[2] == 2, "Incorrect third vertex index");

        require(first.vertices[1].position.x == 1.0f, "Incorrect captured position");
        require(first.vertices[2].uv.y == 1.0f, "Incorrect captured UV");
        require(first.vertices[0].color == ve::makeColor(255, 0, 0), "Incorrect captured color");

        require(selector.select(frame, 800, 600, 0, 1), "Second triangle selection failed");

        const ve::PipelineTriangleSnapshot second = *selector.snapshot();

        require(second.identity.triangleIndex == 1, "Incorrect second triangle number");
        require(second.identity.vertexIndices[0] == 0, "Incorrect second triangle index 0");
        require(second.identity.vertexIndices[1] == 2, "Incorrect second triangle index 1");
        require(second.identity.vertexIndices[2] == 3, "Incorrect second triangle index 2");

        require(second.vertices[2].position.y == 1.0f, "Incorrect final vertex position");
        require(second.vertices[2].color == ve::makeColor(255, 255, 0), "Incorrect final vertex color");

        std::cout << "[PASS] Triangle index resolution and vertex attribute copying\n";
    }


    void testDrawIdentityAndMatrices() {
        ve::MeshData mesh = createQuadMesh();
        ve::RenderFrame frame = createFrame(mesh);

        ve::PipelineTriangleSelector selector;

        require(selector.select(frame, 800, 600, 0, 0), "Chunk selection failed");

        const ve::PipelineTriangleSnapshot chunk = *selector.snapshot();

        require(chunk.identity.drawIndex == 0, "Incorrect Chunk Draw index");
        require(chunk.identity.meshId == 42, "Incorrect Chunk Mesh ID");
        require(chunk.identity.meshRevision == 7, "Incorrect Chunk Mesh revision");
        require(chunk.model(0, 3) == 2.0f, "Incorrect Chunk Model Matrix");

        require(selector.select(frame, 800, 600, 1, 0), "Reference cube selection failed");

        const ve::PipelineTriangleSnapshot cube = *selector.snapshot();

        require(cube.identity.drawIndex == 1, "Incorrect reference cube Draw index");
        require(cube.identity.meshId == 0, "Reference cube must allow Mesh ID zero");
        require(cube.model(0, 3) == 16.0f, "Incorrect reference cube Model Matrix");
        require(cube.view(2, 3) == -5.0f, "Incorrect captured View Matrix");
        require(cube.projection(0, 0) == 1.0f, "Incorrect captured Projection Matrix");

        require(cube.framebufferWidth == 800, "Incorrect framebuffer width");
        require(cube.framebufferHeight == 600, "Incorrect framebuffer height");

        std::cout << "[PASS] Draw identity, Mesh revision and transformation matrices\n";
    }


    void testInvalidSelectionPreservesSnapshot() {
        ve::MeshData mesh = createQuadMesh();
        ve::RenderFrame frame = createFrame(mesh);

        ve::PipelineTriangleSelector selector;

        require(selector.select(frame, 800, 600, 0, 1), "Initial selection failed");

        require(!selector.select(frame, 800, 600, 99, 0), "Invalid Draw index must fail");
        require(!selector.select(frame, 800, 600, 0, 99), "Invalid Triangle index must fail");
        require(!selector.select(frame, 0, 600, 0, 0), "Invalid width must fail");
        require(!selector.select(frame, 800, -1, 0, 0), "Invalid height must fail");

        frame.draws.push_back(ve::RenderDraw{});

        require(!selector.select(frame, 800, 600, 2, 0), "Null Mesh must fail");

        require(selector.hasSelection(), "Failed selection must preserve previous snapshot");
        require(selector.snapshot()->identity.triangleIndex == 1,
                "Failed selection changed the selected triangle");

        std::cout << "[PASS] Invalid selection preserves previous snapshot\n";
    }


    void testSnapshotSurvivesOriginalData() {
        ve::PipelineTriangleSelector selector;

        {
            ve::MeshData mesh = createQuadMesh();
            ve::RenderFrame frame = createFrame(mesh);

            require(selector.select(frame, 800, 600, 0, 0), "Snapshot capture failed");

            frame.draws[0].meshRevision = 100;
            frame.draws[0].model = ve::Mat4::identity();
            frame.view = ve::Mat4::identity();
        }

        require(selector.hasSelection(), "Snapshot lost after source destruction");
        require(selector.snapshot()->identity.meshRevision == 7, "Revision snapshot changed");
        require(selector.snapshot()->model(0, 3) == 2.0f, "Model snapshot changed");
        require(selector.snapshot()->view(2, 3) == -5.0f, "View snapshot changed");

        require(selector.snapshot()->vertices[1].position.x == 1.0f,
                "Vertex data lost after original Mesh destruction");

        std::cout << "[PASS] Snapshot survives RenderFrame and Mesh destruction\n";
    }


    void testTraceIntegrationAndClear() {
        ve::MeshData mesh = createQuadMesh();
        ve::RenderFrame frame = createFrame(mesh);

        ve::PipelineTriangleSelector selector;
        ve::PipelineTrace trace{};

        require(selector.select(frame, 800, 600, 1, 1), "Trace selection failed");

        trace.triangle = selector.snapshot();

        require(trace.triangle.has_value(), "Trace must receive a Triangle Snapshot");
        require(trace.isConsistent(), "Captured Triangle Trace must be consistent");

        selector.clear();

        require(!selector.hasSelection(), "Clear must remove active selection");
        require(!selector.snapshot().has_value(), "Clear must reset snapshot");

        require(trace.triangle.has_value(), "Selector clear must not invalidate copied Trace");
        require(trace.triangle->identity.drawIndex == 1, "Copied Trace identity changed");

        std::cout << "[PASS] PipelineTrace integration and independent snapshot lifetime\n";
    }
}


int main() {
    try {
        testInitialState();
        testTriangleIndicesAndAttributes();
        testDrawIdentityAndMatrices();
        testInvalidSelectionPreservesSnapshot();
        testSnapshotSurvivesOriginalData();
        testTraceIntegrationAndClear();

        std::cout << "\nAll Pipeline Triangle Selector tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
