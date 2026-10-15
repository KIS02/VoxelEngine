
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "render/MeshData.h"
#include "render/RenderFrame.h"
#include "render/pipeline/PipelineTriangleSelector.h"
#include "render/pipeline/PipelineTransformTracer.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    void requireNear( float actual, float expected, const char* message ) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > 0.0001f)
            throw std::runtime_error(message);
    }


    ve::MeshData createTriangleMesh() {
        std::vector<ve::Vertex> vertices(3);

        vertices[0].position = ve::Vec3{ 1.0f, 2.0f, 3.0f };
        vertices[1].position = ve::Vec3{ 0.0f, 0.0f, 0.0f };
        vertices[2].position = ve::Vec3{ -1.0f, 4.0f, -2.0f };

        vertices[0].uv = ve::Vec2{ 0.0f, 0.0f };
        vertices[1].uv = ve::Vec2{ 1.0f, 0.0f };
        vertices[2].uv = ve::Vec2{ 0.0f, 1.0f };

        return ve::MeshData(std::move(vertices), { 0, 1, 2 });
    }


    ve::RenderFrame createFrame( const ve::MeshData& mesh ) {
        ve::RenderFrame frame{};

        frame.draws.push_back(ve::RenderDraw{
            &mesh, ve::Mat4::translation(ve::Vec3{ 10.0f, 20.0f, 30.0f }), 7, 42
        });

        frame.view = ve::Mat4::translation(ve::Vec3{ -2.0f, -3.0f, -4.0f });
        frame.projection = ve::Mat4::identity();

        frame.projection(0, 0) = 2.0f;
        frame.projection(1, 1) = 3.0f;
        frame.projection(2, 2) = 4.0f;

        return frame;
    }


    ve::PipelineTrace createSelectedTrace( const ve::RenderFrame& frame ) {
        ve::PipelineTriangleSelector selector;

        require(selector.select(frame, 800, 600, 0, 0), "Triangle selection failed");

        ve::PipelineTrace trace{};
        trace.triangle = selector.snapshot();

        return trace;
    }


    void testNoTriangleRejected() {
        ve::PipelineTrace trace{};

        require(!ve::PipelineTransformTracer::trace(trace), "Missing triangle must fail");
        require(!trace.worldPositions.has_value(), "Failed trace must not create world values");
        require(trace.isConsistent(), "Empty trace should remain consistent");

        std::cout << "[PASS] Missing triangle is rejected\n";
    }


    void testModelViewProjectionOrder() {
        ve::MeshData mesh = createTriangleMesh();
        ve::RenderFrame frame = createFrame(mesh);
        ve::PipelineTrace trace = createSelectedTrace(frame);

        require(ve::PipelineTransformTracer::trace(trace), "Transform tracing failed");

        const ve::Vec4& world = (*trace.worldPositions)[0];
        const ve::Vec4& view = (*trace.viewPositions)[0];
        const ve::Vec4& clip = (*trace.clipPositions)[0];

        requireNear(world.x, 11.0f, "World X must be Model * Local");
        requireNear(world.y, 22.0f, "World Y is incorrect");
        requireNear(world.z, 33.0f, "World Z is incorrect");
        requireNear(world.w, 1.0f, "World W must remain one");

        requireNear(view.x, 9.0f, "View X must be View * World");
        requireNear(view.y, 19.0f, "View Y is incorrect");
        requireNear(view.z, 29.0f, "View Z is incorrect");
        requireNear(view.w, 1.0f, "View W must remain one");

        requireNear(clip.x, 18.0f, "Clip X must be Projection * View");
        requireNear(clip.y, 57.0f, "Clip Y is incorrect");
        requireNear(clip.z, 116.0f, "Clip Z is incorrect");
        requireNear(clip.w, 1.0f, "Clip W must remain one for the test matrix");

        require(trace.origin == ve::PipelineTraceOrigin::CpuReference, "Trace origin is incorrect");
        require(trace.isConsistent(), "Transform result must be consistent");

        std::cout << "[PASS] Model, View and Projection order\n";
    }


    void testAllThreeVertices() {
        ve::MeshData mesh = createTriangleMesh();
        ve::RenderFrame frame = createFrame(mesh);
        ve::PipelineTrace trace = createSelectedTrace(frame);

        require(ve::PipelineTransformTracer::trace(trace), "All-vertex tracing failed");

        requireNear((*trace.worldPositions)[1].x, 10.0f, "Vertex 1 world X incorrect");
        requireNear((*trace.worldPositions)[1].y, 20.0f, "Vertex 1 world Y incorrect");
        requireNear((*trace.worldPositions)[2].z, 28.0f, "Vertex 2 world Z incorrect");

        requireNear((*trace.viewPositions)[1].z, 26.0f, "Vertex 1 view Z incorrect");
        requireNear((*trace.viewPositions)[2].y, 21.0f, "Vertex 2 view Y incorrect");

        requireNear((*trace.clipPositions)[1].x, 16.0f, "Vertex 1 clip X incorrect");
        requireNear((*trace.clipPositions)[2].y, 63.0f, "Vertex 2 clip Y incorrect");
        requireNear((*trace.clipPositions)[2].z, 96.0f, "Vertex 2 clip Z incorrect");

        require(!trace.clippedPolygon.has_value(), "Clipping must remain uncomputed in 8.3");
        require(!trace.ndcPolygon.has_value(), "NDC must remain uncomputed in 8.3");
        require(!trace.screenPolygon.has_value(), "Screen must remain uncomputed in 8.3");

        std::cout << "[PASS] All three vertices transformed, later stages untouched\n";
    }


    void testClipWIsPreserved() {
        ve::MeshData mesh = createTriangleMesh();
        ve::RenderFrame frame = createFrame(mesh);
        ve::PipelineTrace trace = createSelectedTrace(frame);

        trace.triangle->projection(3, 3) = 0.0f;

        require(ve::PipelineTransformTracer::trace(trace), "Clip W zero must not be rejected");
        requireNear((*trace.clipPositions)[0].w, 0.0f, "Clip W zero must be preserved");

        trace.triangle->projection(3, 3) = -1.0f;

        require(ve::PipelineTransformTracer::trace(trace), "Clip W negative must not be rejected");
        requireNear((*trace.clipPositions)[0].w, -1.0f, "Negative Clip W must be preserved");

        std::cout << "[PASS] Homogeneous Clip W is preserved without divide\n";
    }


    void testInvalidInputDoesNotOverwriteResults() {
        ve::MeshData mesh = createTriangleMesh();
        ve::RenderFrame frame = createFrame(mesh);
        ve::PipelineTrace trace = createSelectedTrace(frame);

        require(ve::PipelineTransformTracer::trace(trace), "Initial valid trace failed");


        // Reject NaN Vertex Position

        trace.triangle->vertices[0].position.x = std::numeric_limits<float>::quiet_NaN();

        require(!ve::PipelineTransformTracer::trace(trace), "NaN position must be rejected");
        requireNear((*trace.worldPositions)[0].x, 11.0f, "Rejected input changed world result");


        // Reject Infinite Matrix

        trace.triangle->vertices[0].position.x = 1.0f;
        trace.triangle->model(0, 0) = std::numeric_limits<float>::infinity();

        require(!ve::PipelineTransformTracer::trace(trace), "Infinite matrix must be rejected");
        requireNear((*trace.clipPositions)[0].x, 18.0f, "Rejected matrix changed clip result");


        // Reject Overflow During Transformation

        trace.triangle->model(0, 0) = std::numeric_limits<float>::max();
        trace.triangle->vertices[0].position.x = 2.0f;

        require(!ve::PipelineTransformTracer::trace(trace),
                "Overflowed transformed coordinate must be rejected");

        requireNear((*trace.worldPositions)[0].x, 11.0f, "Overflow changed previous result");


        // Reject Invalid Framebuffer Dimensions

        trace.triangle->vertices[0].position.x = 1.0f;
        trace.triangle->model(0, 0) = 1.0f;
        trace.triangle->framebufferWidth = 0;

        require(!ve::PipelineTransformTracer::trace(trace), "Invalid framebuffer must be rejected");

        std::cout << "[PASS] Non-finite inputs and overflow preserve earlier results\n";
    }


    void testRetraceInvalidatesDownstream() {
        ve::MeshData mesh = createTriangleMesh();
        ve::RenderFrame frame = createFrame(mesh);
        ve::PipelineTrace trace = createSelectedTrace(frame);

        require(ve::PipelineTransformTracer::trace(trace), "Initial trace failed");


        // Simulate Results From Later Pipeline Stages

        trace.clippedPolygon = std::vector<ve::PipelineClipVertex>(3);
        trace.ndcPolygon = std::vector<ve::Vec3>(3);
        trace.screenPolygon = std::vector<ve::PipelineScreenVertex>(3);
        trace.pixelSample = ve::PipelinePixelSample{};

        require(trace.isConsistent(), "Setup with downstream stages must be valid");


        // Change the Frozen Model Matrix and Recalculate

        trace.origin = ve::PipelineTraceOrigin::SoftwareObserved;
        trace.triangle->model(0, 3) = 11.0f;

        require(ve::PipelineTransformTracer::trace(trace), "Retracing updated triangle failed");
        requireNear((*trace.worldPositions)[0].x, 12.0f, "Retracing did not update world position");


        // Previous Downstream Results Must Be Invalidated

        require(!trace.clippedPolygon.has_value(), "Old clipped polygon must be cleared");
        require(!trace.ndcPolygon.has_value(), "Old NDC polygon must be cleared");
        require(!trace.screenPolygon.has_value(), "Old screen polygon must be cleared");
        require(!trace.pixelSample.has_value(), "Old pixel sample must be cleared");

        require(trace.origin == ve::PipelineTraceOrigin::CpuReference,
                "Retraced data must be CPU reference");

        require(trace.isConsistent(), "Retraced result must be consistent");

        std::cout << "[PASS] Retracing refreshes transforms and clears downstream stages\n";
    }


    void testSnapshotSurvivesMeshDestruction() {
        ve::PipelineTrace trace{};

        {
            ve::MeshData mesh = createTriangleMesh();
            ve::RenderFrame frame = createFrame(mesh);

            trace = createSelectedTrace(frame);
        }


        // Original Mesh and RenderFrame Have Been Destroyed

        require(ve::PipelineTransformTracer::trace(trace),
                "Tracing after source destruction failed");

        requireNear((*trace.worldPositions)[0].x, 11.0f,
                    "Frozen snapshot was invalidated");

        std::cout << "[PASS] Frozen snapshot transforms after Mesh destruction\n";
    }
}


int main() {
    try {
        testNoTriangleRejected();
        testModelViewProjectionOrder();
        testAllThreeVertices();
        testClipWIsPreserved();
        testInvalidInputDoesNotOverwriteResults();
        testRetraceInvalidatesDownstream();
        testSnapshotSurvivesMeshDestruction();

        std::cout << "\nAll Pipeline Transform Tracer tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
