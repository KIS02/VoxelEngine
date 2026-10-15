
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "render/pipeline/PipelineRasterizationTracer.h"


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


    ve::PipelineScreenVertex makeVertex( float x, float y, float u, float v,
                                         float depth, float inverseW = 1.0f ) {
        ve::PipelineScreenVertex vertex{};

        vertex.position = ve::Vec2{ x, y };
        vertex.uv = ve::Vec2{ u, v };
        vertex.depth = depth;
        vertex.inverseW = inverseW;

        return vertex;
    }


    ve::PipelineTrace makeTrace( std::vector<ve::PipelineScreenVertex> polygon ) {
        ve::PipelineTrace trace{};

        ve::PipelineTriangleSnapshot snapshot{};

        snapshot.framebufferWidth = 8;
        snapshot.framebufferHeight = 8;

        trace.triangle = snapshot;

        trace.worldPositions = std::array<ve::Vec4, 3>{};
        trace.viewPositions = std::array<ve::Vec4, 3>{};
        trace.clipPositions = std::array<ve::Vec4, 3>{};

        trace.clippedPolygon = std::vector<ve::PipelineClipVertex>(polygon.size());
        trace.ndcPolygon = std::vector<ve::Vec3>(polygon.size());
        trace.screenPolygon = std::move(polygon);

        require(trace.isConsistent(), "Test Pipeline Trace is inconsistent");

        return trace;
    }


    ve::PipelineTrace makeTriangleTrace() {
        return makeTrace({
            makeVertex(1.0f, 1.0f, 0.0f, 0.0f, 0.1f),
            makeVertex(1.0f, 5.0f, 0.0f, 1.0f, 0.5f),
            makeVertex(5.0f, 1.0f, 1.0f, 0.0f, 0.9f)
        });
    }


    void testMissingStages() {
        ve::PipelineTrace empty{};

        require(!ve::PipelineRasterizationTracer::trace(empty, 2, 2),
                "Empty trace must fail");

        ve::PipelineTrace clipped = makeTrace({});

        require(!ve::PipelineRasterizationTracer::trace(clipped, 2, 2),
                "Fully clipped polygon has no rasterized triangle");

        require(!clipped.pixelSample.has_value(),
                "Missing raster geometry must not create a pixel sample");

        std::cout << "[PASS] Missing and fully clipped geometry are rejected\n";
    }


    void testCoverageAndBarycentric() {
        ve::PipelineTrace trace = makeTriangleTrace();

        require(ve::PipelineRasterizationTracer::trace(trace, 2, 2),
                "Pixel tracing failed");

        require(trace.pixelSample.has_value(), "Pixel sample missing");

        const ve::PipelinePixelSample& sample = *trace.pixelSample;

        require(sample.covered, "Pixel center must be inside the triangle");
        require(sample.x == 2 && sample.y == 2, "Incorrect selected pixel");

        requireNear(sample.barycentric.x, 0.25f, "Incorrect alpha");
        requireNear(sample.barycentric.y, 0.375f, "Incorrect beta");
        requireNear(sample.barycentric.z, 0.375f, "Incorrect gamma");

        const float sum = sample.barycentric.x +
                          sample.barycentric.y +
                          sample.barycentric.z;

        requireNear(sum, 1.0f, "Barycentric weights must sum to one");

        requireNear(sample.uv.x, 0.375f, "Incorrect interpolated U");
        requireNear(sample.uv.y, 0.375f, "Incorrect interpolated V");

        requireNear(sample.incomingDepth, 0.55f, "Incorrect interpolated depth");

        require(!sample.depthPassed, "Depth Test must not run in Phase 8.6");
        requireNear(sample.storedDepth, 1.0f, "Stored depth must remain unobserved default");

        require(trace.isConsistent(), "Covered pixel trace inconsistent");

        std::cout << "[PASS] Pixel coverage, barycentric weights, UV and depth\n";
    }


    void testUncoveredPixel() {
        ve::PipelineTrace trace = makeTriangleTrace();

        require(ve::PipelineRasterizationTracer::trace(trace, 7, 7),
                "Valid uncovered pixel must be analyzed successfully");

        const ve::PipelinePixelSample& sample = *trace.pixelSample;

        require(!sample.covered, "Outside pixel must not be covered");
        require(sample.x == 7 && sample.y == 7, "Outside pixel coordinates incorrect");
        require(!sample.depthPassed, "Outside pixel cannot pass Depth Test");

        std::cout << "[PASS] Uncovered pixel is distinguished from tracing failure\n";
    }


    void testBackfaceAndDegenerateTriangle() {
        ve::PipelineTrace backface = makeTrace({
            makeVertex(1.0f, 1.0f, 0.0f, 0.0f, 0.5f),
            makeVertex(5.0f, 1.0f, 1.0f, 0.0f, 0.5f),
            makeVertex(1.0f, 5.0f, 0.0f, 1.0f, 0.5f)
        });

        require(ve::PipelineRasterizationTracer::trace(backface, 2, 2),
                "Backface tracing failed");

        require(!backface.pixelSample->covered,
                "Backfacing triangle must be culled");

        ve::PipelineTrace degenerate = makeTrace({
            makeVertex(1.0f, 1.0f, 0.0f, 0.0f, 0.5f),
            makeVertex(2.0f, 2.0f, 0.5f, 0.5f, 0.5f),
            makeVertex(3.0f, 3.0f, 1.0f, 1.0f, 0.5f)
        });

        require(ve::PipelineRasterizationTracer::trace(degenerate, 2, 2),
                "Degenerate triangle tracing failed");

        require(!degenerate.pixelSample->covered,
                "Zero-area triangle must not cover pixels");

        std::cout << "[PASS] Backfacing and degenerate triangles are rejected\n";
    }


    void testPerspectiveCorrectUv() {
        ve::PipelineTrace trace = makeTriangleTrace();

        // Vertex C is twice as far in homogeneous W terms.

        (*trace.screenPolygon)[2].inverseW = 0.5f;

        require(ve::PipelineRasterizationTracer::trace(trace, 2, 2),
                "Perspective-correct interpolation failed");

        const ve::PipelinePixelSample& sample = *trace.pixelSample;

        require(sample.covered, "Perspective test pixel must be covered");

        requireNear(sample.uv.x, 3.0f / 13.0f,
                    "Perspective-correct U is incorrect");

        requireNear(sample.uv.y, 6.0f / 13.0f,
                    "Perspective-correct V is incorrect");

        requireNear(sample.incomingDepth, 0.55f,
                    "Screen-space depth interpolation changed unexpectedly");

        require(std::fabs(sample.uv.x - 0.375f) > 0.1f,
                "U appears to use affine interpolation instead");

        std::cout << "[PASS] Perspective-correct UV differs from affine UV\n";
    }


    void testClippedQuadTriangleFan() {
        ve::PipelineTrace trace = makeTrace({
            makeVertex(1.0f, 1.0f, 0.0f, 0.0f, 0.25f),
            makeVertex(1.0f, 6.0f, 0.0f, 1.0f, 0.25f),
            makeVertex(6.0f, 6.0f, 1.0f, 1.0f, 0.25f),
            makeVertex(6.0f, 1.0f, 1.0f, 0.0f, 0.25f)
        });


        // This Pixel Lies Inside the Second Fan Triangle

        require(ve::PipelineRasterizationTracer::trace(trace, 5, 2),
                "Second fan triangle tracing failed");

        require(trace.pixelSample->covered,
                "Second fan triangle must cover the selected pixel");

        requireNear(trace.pixelSample->uv.x, 0.9f,
                    "Second fan triangle U incorrect");

        requireNear(trace.pixelSample->uv.y, 0.3f,
                    "Second fan triangle V incorrect");


        // This Pixel Lies Inside the First Fan Triangle

        require(ve::PipelineRasterizationTracer::trace(trace, 2, 5),
                "First fan triangle tracing failed");

        require(trace.pixelSample->covered,
                "First fan triangle must cover the selected pixel");

        requireNear(trace.pixelSample->uv.x, 0.3f,
                    "First fan triangle U incorrect");

        requireNear(trace.pixelSample->uv.y, 0.9f,
                    "First fan triangle V incorrect");

        std::cout << "[PASS] Clipped quad rasterizes through both fan triangles\n";
    }


    void testRetraceReplacesPreviousSample() {
        ve::PipelineTrace trace = makeTriangleTrace();

        trace.origin = ve::PipelineTraceOrigin::SoftwareObserved;

        require(ve::PipelineRasterizationTracer::trace(trace, 2, 2),
                "Initial pixel sample failed");

        require(trace.pixelSample->covered, "Initial pixel must be covered");


        // Change the Selected Pixel Without Changing the Triangle

        require(ve::PipelineRasterizationTracer::trace(trace, 7, 7),
                "Retracing another pixel failed");

        require(!trace.pixelSample->covered,
                "New uncovered sample must replace the previous covered sample");

        require(trace.pixelSample->x == 7 && trace.pixelSample->y == 7,
                "Old pixel coordinates survived retracing");

        require(trace.origin == ve::PipelineTraceOrigin::SoftwareObserved,
                "Rasterization must preserve trace provenance");

        require(trace.isConsistent(), "Retraced pixel result inconsistent");

        std::cout << "[PASS] Retracing replaces pixel sample and preserves origin\n";
    }


    void testInvalidInputPreservesPreviousSample() {
        ve::PipelineTrace trace = makeTriangleTrace();

        require(ve::PipelineRasterizationTracer::trace(trace, 2, 2),
                "Initial valid sample failed");

        const ve::PipelinePixelSample previous = *trace.pixelSample;


        // Out-of-bounds Coordinates

        require(!ve::PipelineRasterizationTracer::trace(trace, -1, 2),
                "Negative pixel X must fail");

        require(!ve::PipelineRasterizationTracer::trace(trace, 8, 2),
                "Pixel X outside framebuffer must fail");


        // Invalid UV

        (*trace.screenPolygon)[0].uv.x = std::numeric_limits<float>::quiet_NaN();

        require(!ve::PipelineRasterizationTracer::trace(trace, 2, 2),
                "Non-finite UV must fail");

        (*trace.screenPolygon)[0].uv.x = 0.0f;


        // Invalid Reciprocal W

        (*trace.screenPolygon)[0].inverseW = 0.0f;

        require(!ve::PipelineRasterizationTracer::trace(trace, 2, 2),
                "Zero inverse W must fail");


        // The Last Successful Sample Must Remain Untouched

        require(trace.pixelSample.has_value(), "Previous sample disappeared");

        require(trace.pixelSample->covered == previous.covered,
                "Previous coverage changed");

        require(trace.pixelSample->x == previous.x &&
                trace.pixelSample->y == previous.y,
                "Previous pixel coordinates changed");

        requireNear(trace.pixelSample->uv.x, previous.uv.x,
                    "Previous UV was overwritten");

        requireNear(trace.pixelSample->incomingDepth, previous.incomingDepth,
                    "Previous depth was overwritten");

        std::cout << "[PASS] Invalid input preserves the last successful sample\n";
    }
}


int main() {
    try {
        testMissingStages();
        testCoverageAndBarycentric();
        testUncoveredPixel();
        testBackfaceAndDegenerateTriangle();
        testPerspectiveCorrectUv();
        testClippedQuadTriangleFan();
        testRetraceReplacesPreviousSample();
        testInvalidInputPreservesPreviousSample();

        std::cout << "\nAll Pipeline Rasterization Tracer tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
