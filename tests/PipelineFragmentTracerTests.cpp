
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include "render/DepthBuffer.h"
#include "render/Texture2D.h"

#include "render/pipeline/PipelineFragmentTracer.h"
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


    ve::PipelineScreenVertex makeVertex( float x, float y, float u, float v, float depth ) {
        ve::PipelineScreenVertex vertex{};

        vertex.position = ve::Vec2{ x, y };
        vertex.uv = ve::Vec2{ u, v };
        vertex.depth = depth;
        vertex.inverseW = 1.0f;

        return vertex;
    }


    ve::Texture2D createTexture() {
        ve::Texture2D texture(2, 2);

        texture.setPixel(0, 0, ve::makeColor(255, 0, 0));
        texture.setPixel(1, 0, ve::makeColor(0, 255, 0));
        texture.setPixel(0, 1, ve::makeColor(0, 0, 255));
        texture.setPixel(1, 1, ve::makeColor(255, 255, 0));

        return texture;
    }


    ve::PipelineTrace createTrace() {
        ve::PipelineTrace trace{};

        ve::PipelineTriangleSnapshot snapshot{};

        snapshot.framebufferWidth = 8;
        snapshot.framebufferHeight = 8;

        trace.triangle = snapshot;

        trace.worldPositions = std::array<ve::Vec4, 3>{};
        trace.viewPositions = std::array<ve::Vec4, 3>{};
        trace.clipPositions = std::array<ve::Vec4, 3>{};

        trace.clippedPolygon = std::vector<ve::PipelineClipVertex>(3);
        trace.ndcPolygon = std::vector<ve::Vec3>(3);

        trace.screenPolygon = std::vector<ve::PipelineScreenVertex>{
            makeVertex(1.0f, 1.0f, 0.0f, 0.0f, 0.1f),
            makeVertex(1.0f, 5.0f, 0.0f, 1.0f, 0.5f),
            makeVertex(5.0f, 1.0f, 1.0f, 0.0f, 0.9f)
        };

        require(trace.isConsistent(), "Initial Pipeline Trace must be consistent");

        require(ve::PipelineRasterizationTracer::trace(trace, 2, 2),
                "Rasterization setup failed");

        require(trace.pixelSample->covered, "Test pixel must be covered");
        requireNear(trace.pixelSample->incomingDepth, 0.55f, "Unexpected incoming depth");

        return trace;
    }


    void testMissingPixelSample() {
        ve::PipelineTrace trace{};
        ve::Texture2D texture = createTexture();

        require(!ve::PipelineFragmentTracer::trace(trace, 1.0f, texture),
                "Empty trace must fail");

        trace = createTrace();
        trace.pixelSample.reset();

        require(!ve::PipelineFragmentTracer::trace(trace, 1.0f, texture),
                "Missing Rasterization result must fail");

        std::cout << "[PASS] Missing pixel sample is rejected\n";
    }


    void testPassingDepthAndTextureColor() {
        ve::PipelineTrace trace = createTrace();
        ve::Texture2D texture = createTexture();

        ve::DepthBuffer depthBuffer(8, 8);

        depthBuffer.clear(1.0f);
        depthBuffer.setDepth(2, 2, 0.75f);


        // Read the Stored Depth Before the Fragment Is Drawn

        const float storedDepth = depthBuffer.depth(2, 2);

        require(ve::PipelineFragmentTracer::trace(trace, storedDepth, texture),
                "Fragment analysis failed");

        const ve::PipelinePixelSample& sample = *trace.pixelSample;

        require(sample.covered, "Pixel must remain covered");
        require(sample.depthTestEvaluated, "Depth Test must be evaluated");
        require(sample.depthPassed, "Incoming depth 0.55 must pass stored depth 0.75");
        require(sample.textureSampled, "Passing fragment must sample the texture");

        requireNear(sample.incomingDepth, 0.55f, "Incoming depth changed");
        requireNear(sample.storedDepth, 0.75f, "Stored depth incorrect");

        require(sample.candidateColor == ve::makeColor(255, 0, 0),
                "Incorrect nearest texture color");

        requireNear(depthBuffer.depth(2, 2), 0.75f,
                    "Fragment Tracer must not modify the DepthBuffer");

        require(trace.isConsistent(), "Passing Fragment Trace must be consistent");

        std::cout << "[PASS] Passing Depth Test samples texture without modifying DepthBuffer\n";
    }


    void testOccludedFragment() {
        ve::PipelineTrace trace = createTrace();
        ve::Texture2D texture = createTexture();

        require(ve::PipelineFragmentTracer::trace(trace, 0.4f, texture),
                "Occluded Fragment analysis failed");

        const ve::PipelinePixelSample& sample = *trace.pixelSample;

        require(sample.depthTestEvaluated, "Depth Test must run");
        require(!sample.depthPassed, "Depth 0.55 must fail against 0.4");
        require(!sample.textureSampled, "Rejected fragment must skip texture sampling");

        requireNear(sample.storedDepth, 0.4f, "Stored depth incorrect");
        require(sample.candidateColor == ve::makeColor(0, 0, 0),
                "Rejected fragment must not retain a texture color");

        std::cout << "[PASS] Occluded fragment fails Depth Test and skips texture\n";
    }


    void testEqualDepthRejected() {
        ve::PipelineTrace trace = createTrace();
        ve::Texture2D texture = createTexture();

        const float equalDepth = trace.pixelSample->incomingDepth;

        require(ve::PipelineFragmentTracer::trace(trace, equalDepth, texture),
                "Equal-depth analysis failed");

        require(trace.pixelSample->depthTestEvaluated, "Equal depth must be tested");
        require(!trace.pixelSample->depthPassed, "Equal depth must fail strict less-than test");
        require(!trace.pixelSample->textureSampled, "Equal-depth fragment must be discarded");

        std::cout << "[PASS] Equal depth is rejected by strict less-than comparison\n";
    }


    void testUncoveredPixel() {
        ve::PipelineTrace trace = createTrace();
        ve::Texture2D texture = createTexture();

        require(ve::PipelineRasterizationTracer::trace(trace, 7, 7),
                "Uncovered pixel setup failed");

        require(!trace.pixelSample->covered, "Pixel must be outside the triangle");

        require(ve::PipelineFragmentTracer::trace(trace, 0.6f, texture),
                "Uncovered fragment analysis must succeed");

        const ve::PipelinePixelSample& sample = *trace.pixelSample;

        require(!sample.depthTestEvaluated, "Uncovered pixel must not execute Depth Test");
        require(!sample.depthPassed, "Uncovered pixel cannot pass Depth Test");
        require(!sample.textureSampled, "Uncovered pixel must not sample texture");

        requireNear(sample.storedDepth, 0.6f, "Reference stored depth incorrect");
        require(trace.isConsistent(), "Uncovered fragment trace must be consistent");

        std::cout << "[PASS] Uncovered pixel skips Depth Test and Texture Sampling\n";
    }


    void testTextureUvClamping() {
        ve::PipelineTrace trace = createTrace();
        ve::Texture2D texture = createTexture();


        // Texture2D::sampleNearest Clamps UV Coordinates to [0, 1]

        trace.pixelSample->uv = ve::Vec2{ -0.5f, 1.5f };

        require(ve::PipelineFragmentTracer::trace(trace, 1.0f, texture),
                "UV clamping test failed");

        require(trace.pixelSample->depthPassed, "Fragment must pass");
        require(trace.pixelSample->textureSampled, "Texture must be sampled");

        require(trace.pixelSample->candidateColor == ve::makeColor(0, 0, 255),
                "UV clamp must select texture pixel (0, 1)");

        std::cout << "[PASS] Nearest texture sampling clamps UV coordinates\n";
    }


    void testBlackTextureIsNotUncomputed() {
        ve::PipelineTrace trace = createTrace();
        ve::Texture2D texture(1, 1);

        texture.setPixel(0, 0, ve::makeColor(0, 0, 0));

        require(ve::PipelineFragmentTracer::trace(trace, 1.0f, texture),
                "Black texture test failed");

        require(trace.pixelSample->textureSampled,
                "Black color must still count as completed sampling");

        require(trace.pixelSample->candidateColor == ve::makeColor(0, 0, 0),
                "Sampled black color incorrect");

        std::cout << "[PASS] Sampled black color differs from uncomputed texture\n";
    }


    void testRetraceClearsPreviousFragmentState() {
        ve::PipelineTrace trace = createTrace();
        ve::Texture2D texture = createTexture();

        require(ve::PipelineFragmentTracer::trace(trace, 1.0f, texture),
                "First fragment trace failed");

        require(trace.pixelSample->depthPassed, "First fragment must pass");


        // Re-running Phase 8.6 Must Invalidate the Previous Fragment Result

        require(ve::PipelineRasterizationTracer::trace(trace, 2, 2),
                "Rasterization retrace failed");

        require(!trace.pixelSample->depthTestEvaluated,
                "New Rasterization must reset Depth Test status");

        require(!trace.pixelSample->textureSampled,
                "New Rasterization must reset Texture Sampling status");


        // Re-run Phase 8.7 With a Closer Occluder

        require(ve::PipelineFragmentTracer::trace(trace, 0.2f, texture),
                "Second fragment trace failed");

        require(trace.pixelSample->depthTestEvaluated, "Second Depth Test missing");
        require(!trace.pixelSample->depthPassed, "Second Depth Test must fail");
        require(!trace.pixelSample->textureSampled, "Old texture sampling status survived");
        require(trace.pixelSample->candidateColor == ve::makeColor(0, 0, 0),
                "Old candidate color survived failed Depth Test");

        std::cout << "[PASS] Retracing invalidates stale Fragment results\n";
    }


    void testInvalidInputPreservesPreviousResult() {
        ve::PipelineTrace trace = createTrace();
        ve::Texture2D texture = createTexture();

        require(ve::PipelineFragmentTracer::trace(trace, 1.0f, texture),
                "Initial valid fragment trace failed");

        const ve::PipelinePixelSample previous = *trace.pixelSample;


        // Invalid Stored Depth

        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float infinity = std::numeric_limits<float>::infinity();

        require(!ve::PipelineFragmentTracer::trace(trace, nan, texture),
                "NaN stored depth must fail");

        require(!ve::PipelineFragmentTracer::trace(trace, infinity, texture),
                "Infinite stored depth must fail");


        // Invalid Texture Dimensions

        ve::Texture2D emptyTexture(0, 0);

        require(!ve::PipelineFragmentTracer::trace(trace, 1.0f, emptyTexture),
                "Empty texture must fail");


        // Invalid Incoming UV

        trace.pixelSample->uv.x = nan;

        require(!ve::PipelineFragmentTracer::trace(trace, 1.0f, texture),
                "NaN UV must fail");

        trace.pixelSample->uv = previous.uv;


        // The Last Successful Fragment Result Must Be Preserved

        require(trace.pixelSample->depthTestEvaluated == previous.depthTestEvaluated,
                "Previous Depth Test status changed");

        require(trace.pixelSample->depthPassed == previous.depthPassed,
                "Previous Depth Test result changed");

        require(trace.pixelSample->textureSampled == previous.textureSampled,
                "Previous sampling result changed");

        require(trace.pixelSample->candidateColor == previous.candidateColor,
                "Previous fragment color changed");

        requireNear(trace.pixelSample->storedDepth, previous.storedDepth,
                    "Previous stored depth changed");

        std::cout << "[PASS] Invalid inputs preserve the last valid Fragment result\n";
    }
}


int main() {
    try {
        testMissingPixelSample();
        testPassingDepthAndTextureColor();
        testOccludedFragment();
        testEqualDepthRejected();
        testUncoveredPixel();
        testTextureUvClamping();
        testBlackTextureIsNotUncomputed();
        testRetraceClearsPreviousFragmentState();
        testInvalidInputPreservesPreviousResult();

        std::cout << "\nAll Pipeline Fragment Tracer tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
