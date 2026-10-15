
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include "render/pipeline/PipelineClippingTracer.h"
#include "render/pipeline/PipelineTransformTracer.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    bool near( float actual, float expected ) {
        return std::isfinite(actual) && std::fabs(actual - expected) < 0.0001f;
    }


    ve::PipelineTrace makeTrace( const std::array<ve::Vec3, 3>& positions ) {
        ve::PipelineTriangleSnapshot snapshot{};

        snapshot.framebufferWidth = 800;
        snapshot.framebufferHeight = 600;

        for (std::size_t i = 0; i < 3; ++i)
            snapshot.vertices[i].position = positions[i];

        snapshot.vertices[0].uv = ve::Vec2{ 0.0f, 0.0f };
        snapshot.vertices[1].uv = ve::Vec2{ 1.0f, 0.0f };
        snapshot.vertices[2].uv = ve::Vec2{ 0.0f, 1.0f };

        snapshot.vertices[0].color = ve::makeColor(255, 0, 0);
        snapshot.vertices[1].color = ve::makeColor(0, 255, 0);
        snapshot.vertices[2].color = ve::makeColor(0, 0, 255);

        ve::PipelineTrace trace{};
        trace.triangle = snapshot;

        require(ve::PipelineTransformTracer::trace(trace), "Transform setup failed");

        return trace;
    }


    const ve::PipelineClipVertex* findVertex( const std::vector<ve::PipelineClipVertex>& vertices,
                                              float x, float y, float z ) {
        for (const ve::PipelineClipVertex& vertex : vertices) {
            if (near(vertex.position.x, x) && near(vertex.position.y, y) &&
                near(vertex.position.z, z) && near(vertex.position.w, 1.0f))
                return &vertex;
        }

        return nullptr;
    }


    void requireWithinClipVolume( const std::vector<ve::PipelineClipVertex>& polygon ) {
        constexpr float epsilon = 0.0001f;

        for (const ve::PipelineClipVertex& vertex : polygon) {
            const ve::Vec4& p = vertex.position;

            require(p.x >= -p.w - epsilon && p.x <= p.w + epsilon, "Clipped X outside volume");
            require(p.y >= -p.w - epsilon && p.y <= p.w + epsilon, "Clipped Y outside volume");
            require(p.z >= -p.w - epsilon && p.z <= p.w + epsilon, "Clipped Z outside volume");
        }
    }


    void testMissingTransformResults() {
        ve::PipelineTrace trace{};

        require(!ve::PipelineClippingTracer::trace(trace), "Empty trace must fail");

        trace.triangle = ve::PipelineTriangleSnapshot{};
        trace.triangle->framebufferWidth = 800;
        trace.triangle->framebufferHeight = 600;

        require(!ve::PipelineClippingTracer::trace(trace), "Missing Clip Space must fail");
        require(!trace.clippedPolygon.has_value(), "Failed trace must remain uncomputed");

        std::cout << "[PASS] Missing Clip Space data is rejected\n";
    }


    void testFullyInsideTriangle() {
        ve::PipelineTrace trace = makeTrace({
            ve::Vec3{ -0.5f, -0.5f, 0.0f },
            ve::Vec3{ 0.5f, -0.5f, 0.0f },
            ve::Vec3{ 0.0f, 0.5f, 0.0f }
        });

        require(ve::PipelineClippingTracer::trace(trace), "Fully inside triangle failed");
        require(trace.clippedPolygon->size() == 3, "Inside triangle must keep three vertices");

        const auto* first = findVertex(*trace.clippedPolygon, -0.5f, -0.5f, 0.0f);

        require(first != nullptr, "Original vertex missing");
        require(first->color == ve::makeColor(255, 0, 0), "Original color not preserved");
        require(near(first->uv.x, 0.0f) && near(first->uv.y, 0.0f), "Original UV not preserved");
        require(trace.isConsistent(), "Inside triangle trace inconsistent");

        std::cout << "[PASS] Fully visible triangle preserves vertices and attributes\n";
    }


    void testPartialClippingAndUvInterpolation() {
        ve::PipelineTrace trace = makeTrace({
            ve::Vec3{ -2.0f, 0.0f, 0.0f },
            ve::Vec3{ 0.0f, 0.8f, 0.0f },
            ve::Vec3{ 0.0f, -0.8f, 0.0f }
        });

        require(ve::PipelineClippingTracer::trace(trace), "Partial clipping failed");
        require(trace.clippedPolygon->size() == 4, "One clipped vertex must produce four vertices");

        requireWithinClipVolume(*trace.clippedPolygon);

        const auto* upper = findVertex(*trace.clippedPolygon, -1.0f, 0.4f, 0.0f);
        const auto* lower = findVertex(*trace.clippedPolygon, -1.0f, -0.4f, 0.0f);

        require(upper != nullptr && lower != nullptr, "Clipping intersections are missing");
        require(near(upper->uv.x, 0.5f) && near(upper->uv.y, 0.0f), "Upper UV interpolation wrong");
        require(near(lower->uv.x, 0.0f) && near(lower->uv.y, 0.5f), "Lower UV interpolation wrong");

        std::cout << "[PASS] Partial clipping generates intersections and interpolates UV\n";
    }


    void testFullyDiscardedTriangle() {
        ve::PipelineTrace trace = makeTrace({
            ve::Vec3{ 2.0f, 0.0f, 0.0f },
            ve::Vec3{ 3.0f, 0.5f, 0.0f },
            ve::Vec3{ 2.0f, -0.5f, 0.0f }
        });

        require(!trace.clippedPolygon.has_value(), "Clipping must initially be uncomputed");

        require(ve::PipelineClippingTracer::trace(trace), "Discarded triangle is a valid result");

        require(trace.clippedPolygon.has_value() && trace.clippedPolygon->empty(),
                "Discarded triangle must have an empty computed polygon");

        require(trace.isConsistent(), "Discarded triangle must be consistent");

        std::cout << "[PASS] Fully clipped triangle differs from uncomputed clipping\n";
    }


    void testClippingAgainstMultiplePlanes() {
        ve::PipelineTrace trace = makeTrace({
            ve::Vec3{ -2.0f, -2.0f, -2.0f },
            ve::Vec3{ 0.7f, 0.1f, 0.0f },
            ve::Vec3{ 0.1f, 0.7f, 0.0f }
        });

        require(ve::PipelineClippingTracer::trace(trace), "Multiple-plane clipping failed");
        require(trace.clippedPolygon->size() >= 3, "Expected a visible clipped polygon");

        requireWithinClipVolume(*trace.clippedPolygon);

        std::cout << "[PASS] Multiple clip planes constrain the output polygon\n";
    }


    void testRetraceInvalidatesLaterStages() {
        ve::PipelineTrace trace = makeTrace({
            ve::Vec3{ -0.5f, -0.5f, 0.0f },
            ve::Vec3{ 0.5f, -0.5f, 0.0f },
            ve::Vec3{ 0.0f, 0.5f, 0.0f }
        });

        require(ve::PipelineClippingTracer::trace(trace), "Initial clipping failed");

        trace.ndcPolygon = std::vector<ve::Vec3>(3);
        trace.screenPolygon = std::vector<ve::PipelineScreenVertex>(3);
        trace.pixelSample = ve::PipelinePixelSample{};
        trace.origin = ve::PipelineTraceOrigin::SoftwareObserved;

        require(trace.isConsistent(), "Downstream test setup invalid");
        require(ve::PipelineClippingTracer::trace(trace), "Clipping rerun failed");

        require(!trace.ndcPolygon.has_value(), "Old NDC data survived retrace");
        require(!trace.screenPolygon.has_value(), "Old Screen data survived retrace");
        require(!trace.pixelSample.has_value(), "Old pixel data survived retrace");

        require(trace.origin == ve::PipelineTraceOrigin::SoftwareObserved,
                "Clipping must not silently change the trace provenance");

        require(trace.isConsistent(), "Retraced result inconsistent");

        std::cout << "[PASS] Retracing invalidates downstream stages, preserves origin\n";
    }


    void testNonFiniteInputPreservesOldPolygon() {
        ve::PipelineTrace trace = makeTrace({
            ve::Vec3{ -0.5f, -0.5f, 0.0f },
            ve::Vec3{ 0.5f, -0.5f, 0.0f },
            ve::Vec3{ 0.0f, 0.5f, 0.0f }
        });

        require(ve::PipelineClippingTracer::trace(trace), "Initial clipping failed");

        const std::vector<ve::PipelineClipVertex> oldPolygon = *trace.clippedPolygon;

        (*trace.clipPositions)[0].x = std::numeric_limits<float>::quiet_NaN();

        require(!ve::PipelineClippingTracer::trace(trace), "NaN Clip Space must fail");

        require(trace.clippedPolygon->size() == oldPolygon.size(),
                "Invalid input changed polygon size");

        require(near((*trace.clippedPolygon)[0].position.x, oldPolygon[0].position.x),
                "Invalid input overwrote last successful polygon");

        std::cout << "[PASS] Non-finite input does not overwrite last valid polygon\n";
    }
}


int main() {
    try {
        testMissingTransformResults();
        testFullyInsideTriangle();
        testPartialClippingAndUvInterpolation();
        testFullyDiscardedTriangle();
        testClippingAgainstMultiplePlanes();
        testRetraceInvalidatesLaterStages();
        testNonFiniteInputPreservesOldPolygon();

        std::cout << "\nAll Pipeline Clipping Tracer tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
