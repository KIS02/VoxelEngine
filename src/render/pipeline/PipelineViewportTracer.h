
#pragma once

#include <cmath>
#include <utility>
#include <vector>

#include "math/Vec2.h"
#include "math/Vec3.h"
#include "math/Vec4.h"

#include "render/RasterVertex.h"
#include "render/Viewport.h"
#include "render/pipeline/PipelineTrace.h"


namespace ve
{
    class PipelineViewportTracer {
        private:
            static bool isFinite( const Vec2& value ) {
                return std::isfinite(value.x) && std::isfinite(value.y);
            }

            static bool isFinite( const Vec3& value ) {
                return std::isfinite(value.x) && std::isfinite(value.y) &&
                       std::isfinite(value.z);
            }

            static bool isFinite( const Vec4& value ) {
                return std::isfinite(value.x) && std::isfinite(value.y) &&
                       std::isfinite(value.z) && std::isfinite(value.w);
            }


        public:
            static bool trace( PipelineTrace& result ) {
                if (!result.triangle.has_value() || !result.clippedPolygon.has_value())
                    return false;

                if (!result.isConsistent())
                    return false;

                const PipelineTriangleSnapshot& snapshot = *result.triangle;
                const std::vector<PipelineClipVertex>& polygon = *result.clippedPolygon;

                Viewport viewport(snapshot.framebufferWidth, snapshot.framebufferHeight);

                std::vector<Vec3> ndc;
                std::vector<PipelineScreenVertex> screen;

                ndc.reserve(polygon.size());
                screen.reserve(polygon.size());


                // Convert Every Clipped Vertex Into NDC and Screen Space

                for (const PipelineClipVertex& vertex : polygon) {
                    if (!isFinite(vertex.position) || !isFinite(vertex.uv))
                        return false;


                    // Perspective Divide Requires a Positive W

                    if (vertex.position.w <= 0.0f)
                        return false;

                    const float inverseW = 1.0f / vertex.position.w;

                    if (!std::isfinite(inverseW))
                        return false;


                    // Clip Space -> Normalized Device Coordinates

                    const Vec3 ndcPosition{
                        vertex.position.x * inverseW,
                        vertex.position.y * inverseW,
                        vertex.position.z * inverseW
                    };

                    if (!isFinite(ndcPosition))
                        return false;


                    // Reuse the Existing Viewport Transformation

                    const RasterVertex rasterVertex = viewport.transform(
                        ndcPosition, vertex.color
                    );

                    if (!isFinite(rasterVertex.position) || !std::isfinite(rasterVertex.depth))
                        return false;


                    // Preserve Interpolation Inputs for Phase 8.6

                    PipelineScreenVertex captured{};

                    captured.position = rasterVertex.position;
                    captured.uv = vertex.uv;
                    captured.depth = rasterVertex.depth;
                    captured.inverseW = inverseW;

                    ndc.push_back(ndcPosition);
                    screen.push_back(captured);
                }


                // Commit Both Stages Together

                PipelineTrace updated = result;

                updated.ndcPolygon = std::move(ndc);
                updated.screenPolygon = std::move(screen);


                // Previous Pixel Analysis Is Now Invalid

                updated.pixelSample.reset();

                if (!updated.isConsistent())
                    return false;

                result = std::move(updated);

                return true;
            }
    };
}
