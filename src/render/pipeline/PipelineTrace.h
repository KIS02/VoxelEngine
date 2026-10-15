
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "math/Mat4.h"
#include "math/Vec2.h"
#include "math/Vec3.h"
#include "math/Vec4.h"
#include "render/Color.h"


namespace ve
{
    enum class PipelineTraceOrigin {
        CpuReference,
        SoftwareObserved
    };


    struct PipelineInputVertex
    {
        Vec3 position{};
        Vec2 uv{};
        Color32 color = makeColor(255, 255, 255);
    };


    struct PipelineTriangleIdentity
    {
        std::size_t drawIndex = 0;
        std::size_t triangleIndex = 0;

        std::uint64_t meshId = 0;
        std::uint64_t meshRevision = 0;

        std::array<std::uint32_t, 3> vertexIndices{};
    };


    struct PipelineTriangleSnapshot
    {
        PipelineTriangleIdentity identity{};
        std::array<PipelineInputVertex, 3> vertices{};

        Mat4 model = Mat4::identity();
        Mat4 view = Mat4::identity();
        Mat4 projection = Mat4::identity();

        int framebufferWidth = 0;
        int framebufferHeight = 0;
    };


    struct PipelineClipVertex
    {
        Vec4 position{};
        Vec2 uv{};
        Color32 color = makeColor(255, 255, 255);
    };


    struct PipelineScreenVertex
    {
        Vec2 position{};
        Vec2 uv{};

        float depth = 1.0f;
        float inverseW = 0.0f;
    };


    struct PipelinePixelSample
    {
        int x = 0;
        int y = 0;

        bool covered = false;
        bool depthTestEvaluated = false;
        bool depthPassed = false;
        bool textureSampled = false;

        Vec3 barycentric{};
        Vec2 uv{};

        float incomingDepth = 1.0f;
        float storedDepth = 1.0f;

        Color32 candidateColor = makeColor(0, 0, 0);
    };


    struct PipelineTrace
    {
        PipelineTraceOrigin origin = PipelineTraceOrigin::CpuReference;

        std::optional<PipelineTriangleSnapshot> triangle{};

        std::optional<std::array<Vec4, 3>> worldPositions{};
        std::optional<std::array<Vec4, 3>> viewPositions{};
        std::optional<std::array<Vec4, 3>> clipPositions{};

        std::optional<std::vector<PipelineClipVertex>> clippedPolygon{};
        std::optional<std::vector<Vec3>> ndcPolygon{};
        std::optional<std::vector<PipelineScreenVertex>> screenPolygon{};

        std::optional<PipelinePixelSample> pixelSample{};


        bool isConsistent() const {
            if (!triangle.has_value()) {
                return !worldPositions.has_value() && !viewPositions.has_value() &&
                       !clipPositions.has_value() && !clippedPolygon.has_value() &&
                       !ndcPolygon.has_value() && !screenPolygon.has_value() &&
                       !pixelSample.has_value();
            }

            if (triangle->framebufferWidth <= 0 || triangle->framebufferHeight <= 0)
                return false;

            if (viewPositions.has_value() && !worldPositions.has_value())
                return false;

            if (clipPositions.has_value() && !viewPositions.has_value())
                return false;

            if (clippedPolygon.has_value() && !clipPositions.has_value())
                return false;

            if (ndcPolygon.has_value() &&
                (!clippedPolygon.has_value() || ndcPolygon->size() != clippedPolygon->size()))
                return false;

            if (screenPolygon.has_value() &&
                (!ndcPolygon.has_value() || screenPolygon->size() != ndcPolygon->size()))
                return false;

            if (pixelSample.has_value() &&
                (!screenPolygon.has_value() || screenPolygon->size() < 3))
                return false;

            if (pixelSample.has_value()) {
                const PipelinePixelSample& sample = *pixelSample;

                if (!sample.covered &&
                    (sample.depthTestEvaluated || sample.depthPassed || sample.textureSampled))
                    return false;

                if (sample.depthPassed && !sample.depthTestEvaluated)
                    return false;

                if (sample.textureSampled && !sample.depthPassed)
                    return false;
            }

            return true;
        }


        void reset() {
            *this = PipelineTrace{};
        }
    };
}
