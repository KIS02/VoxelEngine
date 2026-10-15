
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include "render/Viewport.h"
#include "render/pipeline/PipelineClippingTracer.h"
#include "render/pipeline/PipelineTransformTracer.h"
#include "render/pipeline/PipelineViewportTracer.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    void requireNear( float actual, float expected, const char* message ) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > 0.001f)
            throw std::runtime_error(message);
    }


    ve::PipelineTrace createPreparedTrace( const std::array<ve::Vec3, 3>& positions,
                                           float projectionW = 1.0f ) {
        ve::PipelineTriangleSnapshot snapshot{};

        snapshot.framebufferWidth = 800;
        snapshot.framebufferHeight = 600;

        snapshot.projection(3, 3) = projectionW;

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
        require(ve::PipelineClippingTracer::trace(trace), "Clipping setup failed");

        return trace;
    }


    void requireViewportMatches( const ve::PipelineTrace& trace ) {
        require(trace.ndcPolygon.has_value(), "Missing NDC polygon");
        require(trace.screenPolygon.has_value(), "Missing screen polygon");

        const auto& clipped = *trace.clippedPolygon;
        const auto& ndc = *trace.ndcPolygon;
        const auto& screen = *trace.screenPolygon;

        require(clipped.size() == ndc.size(), "Clipped/NDC count mismatch");
        require(ndc.size() == screen.size(), "NDC/Screen count mismatch");

        const ve::Viewport viewport(
            trace.triangle->framebufferWidth, trace.triangle->framebufferHeight
        );

        for (std::size_t i = 0; i < clipped.size(); ++i) {
            const ve::Vec4& clip = clipped[i].position;

            require(clip.w > 0.0f, "Clipped W must be positive");

            const float inverseW = 1.0f / clip.w;

            requireNear(ndc[i].x, clip.x * inverseW, "NDC X mismatch");
            requireNear(ndc[i].y, clip.y * inverseW, "NDC Y mismatch");
            requireNear(ndc[i].z, clip.z * inverseW, "NDC Z mismatch");

            const ve::RasterVertex expected = viewport.transform(ndc[i], clipped[i].color);

            requireNear(screen[i].position.x, expected.position.x, "Screen X mismatch");
            requireNear(screen[i].position.y, expected.position.y, "Screen Y mismatch");
            requireNear(screen[i].depth, expected.depth, "Screen depth mismatch");

            requireNear(screen[i].inverseW, inverseW, "Inverse W mismatch");

            requireNear(screen[i].uv.x, clipped[i].uv.x, "Screen UV X mismatch");
            requireNear(screen[i].uv.y, clipped[i].uv.y, "Screen UV Y mismatch");
        }
    }


    void testMissingClippingRejected() {
        ve::PipelineTrace empty{};

        require(!ve::PipelineViewportTracer::trace(empty),
                "Empty trace must fail");

        ve::PipelineTrace trace = createPreparedTrace({
            ve::Vec3{ -0.5f, -0.5f, 0.0f },
            ve::Vec3{ 0.5f, -0.5f, 0.0f },
            ve::Vec3{ 0.0f, 0.5f, 0.0f }
        });

        trace.clippedPolygon.reset();

        require(!ve::PipelineViewportTracer::trace(trace),
                "Uncomputed clipping must fail");

        require(!trace.ndcPolygon.has_value(), "Failed trace created NDC");
        require(!trace.screenPolygon.has_value(), "Failed trace created screen data");

        std::cout << "[PASS] Missing clipping result is rejected\n";
    }


    void testNdcAndScreenConversion() {
        ve::PipelineTrace trace = createPreparedTrace({
            ve::Vec3{ -0.5f, -0.5f, 0.0f },
            ve::Vec3{ 0.5f, -0.5f, 0.5f },
            ve::Vec3{ 0.0f, 0.0f, 0.0f }
        });

        require(ve::PipelineViewportTracer::trace(trace),
                "Viewport tracing failed");

        require(trace.ndcPolygon->size() == 3, "Expected three NDC vertices");
        require(trace.screenPolygon->size() == 3, "Expected three screen vertices");

        requireViewportMatches(trace);

        bool foundCenter = false;

        for (std::size_t i = 0; i < trace.ndcPolygon->size(); ++i) {
            const ve::Vec3& ndc = (*trace.ndcPolygon)[i];

            if (std::fabs(ndc.x) < 0.0001f && std::fabs(ndc.y) < 0.0001f) {
                const ve::Vec2& screen = (*trace.screenPolygon)[i].position;

                require(std::fabs(screen.x - 400.0f) <= 1.0f,
                        "NDC center must map near framebuffer center X");

                require(std::fabs(screen.y - 300.0f) <= 1.0f,
                        "NDC center must map near framebuffer center Y");

                foundCenter = true;
            }
        }

        require(foundCenter, "Center vertex missing");
        require(trace.isConsistent(), "Viewport trace must be consistent");

        std::cout << "[PASS] NDC and screen coordinates match the engine viewport\n";
    }


    void testPerspectiveDivideAndInverseW() {
        ve::PipelineTrace trace = createPreparedTrace({
            ve::Vec3{ 1.0f, -0.5f, 0.5f },
            ve::Vec3{ -1.0f, -0.5f, -0.5f },
            ve::Vec3{ 0.0f, 1.0f, 0.0f }
        }, 2.0f);

        require(ve::PipelineViewportTracer::trace(trace),
                "Perspective divide failed");

        requireViewportMatches(trace);

        bool foundVertex = false;

        for (std::size_t i = 0; i < trace.clippedPolygon->size(); ++i) {
            const ve::Vec4& clip = (*trace.clippedPolygon)[i].position;

            if (std::fabs(clip.x - 1.0f) < 0.0001f &&
                std::fabs(clip.y + 0.5f) < 0.0001f) {

                const ve::Vec3& ndc = (*trace.ndcPolygon)[i];
                const ve::PipelineScreenVertex& screen = (*trace.screenPolygon)[i];

                requireNear(clip.w, 2.0f, "Expected homogeneous W of two");

                requireNear(ndc.x, 0.5f, "Perspective divided X incorrect");
                requireNear(ndc.y, -0.25f, "Perspective divided Y incorrect");
                requireNear(ndc.z, 0.25f, "Perspective divided Z incorrect");

                requireNear(screen.inverseW, 0.5f, "Inverse W must be 0.5");

                foundVertex = true;
            }
        }

        require(foundVertex, "Expected perspective test vertex missing");

        std::cout << "[PASS] Perspective divide preserves inverse W and UV\n";
    }


    void testFourVertexClippedPolygon() {
        ve::PipelineTrace trace = createPreparedTrace({
            ve::Vec3{ -2.0f, 0.0f, 0.0f },
            ve::Vec3{ 0.0f, 0.8f, 0.0f },
            ve::Vec3{ 0.0f, -0.8f, 0.0f }
        });

        require(trace.clippedPolygon->size() == 4,
                "Partial clipping must create four vertices");

        require(ve::PipelineViewportTracer::trace(trace),
                "Four-vertex polygon transformation failed");

        require(trace.ndcPolygon->size() == 4, "NDC must retain four vertices");
        require(trace.screenPolygon->size() == 4, "Screen must retain four vertices");

        requireViewportMatches(trace);
        require(trace.isConsistent(), "Four-vertex trace inconsistent");

        std::cout << "[PASS] Four-vertex clipped polygon retains all vertices\n";
    }


    void testFullyClippedTriangle() {
        ve::PipelineTrace trace = createPreparedTrace({
            ve::Vec3{ 2.0f, 0.0f, 0.0f },
            ve::Vec3{ 3.0f, 0.4f, 0.0f },
            ve::Vec3{ 2.0f, -0.4f, 0.0f }
        });

        require(trace.clippedPolygon.has_value() && trace.clippedPolygon->empty(),
                "Test setup must create fully clipped triangle");

        require(ve::PipelineViewportTracer::trace(trace),
                "Fully clipped polygon should be a valid result");

        require(trace.ndcPolygon.has_value() && trace.ndcPolygon->empty(),
                "Fully clipped polygon must produce empty NDC");

        require(trace.screenPolygon.has_value() && trace.screenPolygon->empty(),
                "Fully clipped polygon must produce empty screen polygon");

        require(!trace.pixelSample.has_value(), "Discarded triangle cannot have a pixel sample");
        require(trace.isConsistent(), "Discarded triangle should be consistent");

        std::cout << "[PASS] Fully clipped polygon yields computed empty results\n";
    }


    void testResizeRetraceInvalidatesPixel() {
        ve::PipelineTrace trace = createPreparedTrace({
            ve::Vec3{ -0.5f, -0.5f, 0.0f },
            ve::Vec3{ 0.5f, -0.5f, 0.0f },
            ve::Vec3{ 0.0f, 0.5f, 0.0f }
        });

        require(ve::PipelineViewportTracer::trace(trace),
                "Initial viewport trace failed");

        const float oldScreenX = (*trace.screenPolygon)[0].position.x;
        const ve::Vec3 oldNdc = (*trace.ndcPolygon)[0];

        trace.pixelSample = ve::PipelinePixelSample{};
        trace.origin = ve::PipelineTraceOrigin::SoftwareObserved;

        trace.triangle->framebufferWidth = 400;
        trace.triangle->framebufferHeight = 300;

        require(ve::PipelineViewportTracer::trace(trace),
                "Viewport resize retrace failed");

        require((*trace.screenPolygon)[0].position.x != oldScreenX,
                "Resizing must update screen coordinates");

        requireNear((*trace.ndcPolygon)[0].x, oldNdc.x,
                    "Resizing must not change NDC X");

        requireNear((*trace.ndcPolygon)[0].y, oldNdc.y,
                    "Resizing must not change NDC Y");

        require(!trace.pixelSample.has_value(),
                "Recalculated screen positions must invalidate pixel analysis");

        require(trace.origin == ve::PipelineTraceOrigin::SoftwareObserved,
                "Viewport retrace must preserve trace origin");

        require(trace.isConsistent(), "Resized trace must be consistent");

        std::cout << "[PASS] Resize updates screen space and invalidates pixel sample\n";
    }


    void testInvalidInputsPreservePreviousResult() {
        ve::PipelineTrace trace = createPreparedTrace({
            ve::Vec3{ -0.5f, -0.5f, 0.0f },
            ve::Vec3{ 0.5f, -0.5f, 0.0f },
            ve::Vec3{ 0.0f, 0.5f, 0.0f }
        });

        require(ve::PipelineViewportTracer::trace(trace),
                "Initial trace failed");

        const std::vector<ve::Vec3> oldNdc = *trace.ndcPolygon;
        const std::vector<ve::PipelineScreenVertex> oldScreen = *trace.screenPolygon;

        const ve::PipelineClipVertex original = (*trace.clippedPolygon)[0];


        // Zero W

        (*trace.clippedPolygon)[0].position.w = 0.0f;

        require(!ve::PipelineViewportTracer::trace(trace),
                "Zero W must be rejected");


        // Negative W

        (*trace.clippedPolygon)[0].position.w = -1.0f;

        require(!ve::PipelineViewportTracer::trace(trace),
                "Negative W must be rejected");


        // Non-finite UV

        (*trace.clippedPolygon)[0] = original;
        (*trace.clippedPolygon)[0].uv.x = std::numeric_limits<float>::quiet_NaN();

        require(!ve::PipelineViewportTracer::trace(trace),
                "Non-finite UV must be rejected");


        // Reciprocal Overflow

        (*trace.clippedPolygon)[0] = original;
        (*trace.clippedPolygon)[0].position.w = std::numeric_limits<float>::denorm_min();

        require(!ve::PipelineViewportTracer::trace(trace),
                "Overflowed inverse W must be rejected");


        // Invalid Framebuffer Dimensions

        (*trace.clippedPolygon)[0] = original;
        trace.triangle->framebufferWidth = 0;

        require(!ve::PipelineViewportTracer::trace(trace),
                "Zero framebuffer width must be rejected");


        // Previous Results Must Remain Unchanged

        require(trace.ndcPolygon->size() == oldNdc.size(),
                "Invalid input changed NDC count");

        require(trace.screenPolygon->size() == oldScreen.size(),
                "Invalid input changed screen count");

        for (std::size_t i = 0; i < oldNdc.size(); ++i) {
            requireNear((*trace.ndcPolygon)[i].x, oldNdc[i].x,
                        "Invalid input overwrote previous NDC");

            requireNear((*trace.screenPolygon)[i].position.x, oldScreen[i].position.x,
                        "Invalid input overwrote previous screen result");
        }

        std::cout << "[PASS] Invalid W, UV and framebuffer preserve prior results\n";
    }
}


int main() {
    try {
        testMissingClippingRejected();
        testNdcAndScreenConversion();
        testPerspectiveDivideAndInverseW();
        testFourVertexClippedPolygon();
        testFullyClippedTriangle();
        testResizeRetraceInvalidatesPixel();
        testInvalidInputsPreservePreviousResult();

        std::cout << "\nAll Pipeline Viewport Tracer tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
