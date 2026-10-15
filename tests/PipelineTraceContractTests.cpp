
#include <iostream>
#include <stdexcept>
#include <vector>

#include "render/pipeline/PipelineTrace.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    ve::PipelineTriangleSnapshot createSampleTriangle() {
        ve::PipelineTriangleSnapshot snapshot{};

        snapshot.identity.drawIndex = 4;
        snapshot.identity.triangleIndex = 2;
        snapshot.identity.meshId = 0;
        snapshot.identity.meshRevision = 0;
        snapshot.identity.vertexIndices = { 0, 1, 2 };

        snapshot.vertices[0].position = ve::Vec3{ 1.0f, 0.0f, 0.0f };
        snapshot.vertices[1].position = ve::Vec3{ 0.0f, 1.0f, 0.0f };
        snapshot.vertices[2].position = ve::Vec3{ 0.0f, 0.0f, 1.0f };

        snapshot.framebufferWidth = 800;
        snapshot.framebufferHeight = 600;

        return snapshot;
    }


    void attachTransformResults( ve::PipelineTrace& trace ) {
        trace.worldPositions = std::array<ve::Vec4, 3>{};
        trace.viewPositions = std::array<ve::Vec4, 3>{};
        trace.clipPositions = std::array<ve::Vec4, 3>{};
    }


    void testEmptyTrace() {
        ve::PipelineTrace trace{};

        require(!trace.triangle.has_value(), "Empty trace must not have a triangle");
        require(!trace.worldPositions.has_value(), "Empty trace must not contain transformations");
        require(!trace.clippedPolygon.has_value(), "Clipping must initially be uncomputed");
        require(trace.isConsistent(), "Empty trace must be consistent");

        std::cout << "[PASS] Empty trace and uncomputed stages\n";
    }


    void testSnapshotOwnsTriangleData() {
        ve::PipelineTriangleSnapshot snapshot = createSampleTriangle();
        ve::PipelineTrace trace{};

        trace.triangle = snapshot;

        snapshot.vertices[0].position.x = 99.0f;
        snapshot.identity.vertexIndices[0] = 99;

        require(trace.triangle->vertices[0].position.x == 1.0f,
                "Snapshot must own original vertex values");

        require(trace.triangle->identity.vertexIndices[0] == 0,
                "Snapshot must own original mesh indices");

        require(trace.triangle->identity.meshId == 0,
                "Reference cube must permit Mesh ID zero");

        require(trace.isConsistent(), "Valid selection must be consistent");

        std::cout << "[PASS] Triangle snapshot copies values and supports Mesh ID zero\n";
    }


    void testStageDependencies() {
        ve::PipelineTrace trace{};
        trace.triangle = createSampleTriangle();

        trace.viewPositions = std::array<ve::Vec4, 3>{};

        require(!trace.isConsistent(), "View Space requires World Space");

        trace.worldPositions = std::array<ve::Vec4, 3>{};

        require(trace.isConsistent(), "World and View stages should be valid");

        trace.clipPositions = std::array<ve::Vec4, 3>{};
        trace.clippedPolygon = std::vector<ve::PipelineClipVertex>(3);
        trace.ndcPolygon = std::vector<ve::Vec3>(2);

        require(!trace.isConsistent(), "Clipped and NDC vertex counts must match");

        trace.ndcPolygon->push_back(ve::Vec3{});
        trace.screenPolygon = std::vector<ve::PipelineScreenVertex>(3);
        trace.pixelSample = ve::PipelinePixelSample{};

        require(trace.isConsistent(), "Completed trace stages should be consistent");

        std::cout << "[PASS] Stage dependencies and polygon vertex counts\n";
    }


    void testFullyClippedPolygon() {
        ve::PipelineTrace trace{};

        trace.triangle = createSampleTriangle();
        attachTransformResults(trace);

        require(!trace.clippedPolygon.has_value(), "Clipping is not computed yet");

        trace.clippedPolygon = std::vector<ve::PipelineClipVertex>{};
        trace.ndcPolygon = std::vector<ve::Vec3>{};
        trace.screenPolygon = std::vector<ve::PipelineScreenVertex>{};

        require(trace.clippedPolygon.has_value() && trace.clippedPolygon->empty(),
                "Empty clipped polygon must mean entirely discarded triangle");

        require(trace.isConsistent(), "Fully clipped triangle is a valid trace result");

        trace.pixelSample = ve::PipelinePixelSample{};

        require(!trace.isConsistent(), "Discarded triangle must not produce a pixel sample");

        std::cout << "[PASS] Uncomputed clipping and fully discarded triangle are distinct\n";
    }


    void testReset() {
        ve::PipelineTrace trace{};

        trace.triangle = createSampleTriangle();
        attachTransformResults(trace);
        trace.clippedPolygon = std::vector<ve::PipelineClipVertex>{};

        trace.reset();

        require(!trace.triangle.has_value(), "Reset must clear triangle snapshot");
        require(!trace.clipPositions.has_value(), "Reset must clear transformations");
        require(!trace.clippedPolygon.has_value(), "Reset must clear clipped result");
        require(trace.isConsistent(), "Reset trace must be consistent");

        std::cout << "[PASS] Trace reset clears previous selection and results\n";
    }
}


int main() {
    try {
        testEmptyTrace();
        testSnapshotOwnsTriangleData();
        testStageDependencies();
        testFullyClippedPolygon();
        testReset();

        std::cout << "\nAll Pipeline Trace Contract tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
