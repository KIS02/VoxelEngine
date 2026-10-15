
#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include "render/ClipSpaceClipper.h"
#include "render/pipeline/PipelineTrace.h"


namespace ve
{
    class PipelineClippingTracer {
        private:
            static bool isFinite( const Vec4& position ) {
                return std::isfinite(position.x) && std::isfinite(position.y) &&
                       std::isfinite(position.z) && std::isfinite(position.w);
            }

            static bool isFinite( const Vec2& uv ) {
                return std::isfinite(uv.x) && std::isfinite(uv.y);
            }


        public:
            static bool trace( PipelineTrace& result ) {
                if (!result.triangle.has_value() || !result.clipPositions.has_value())
                    return false;

                if (!result.isConsistent())
                    return false;

                const PipelineTriangleSnapshot& snapshot = *result.triangle;
                const std::array<Vec4, 3>& clipPositions = *result.clipPositions;


                // Prepare the Original Clip Space Vertices

                std::array<ClipVertex, 3> inputs{};

                for (std::size_t i = 0; i < 3; ++i) {
                    if (!isFinite(clipPositions[i]) || !isFinite(snapshot.vertices[i].uv))
                        return false;

                    inputs[i].position = clipPositions[i];
                    inputs[i].uv = snapshot.vertices[i].uv;
                    inputs[i].color = snapshot.vertices[i].color;
                }


                // Reuse the Existing Software Clipping Algorithm

                std::array<ClipVertex, 16> clipped{};

                const std::size_t count = ClipSpaceClipper::clipTriangle(
                    inputs[0], inputs[1], inputs[2], clipped
                );

                if (count > clipped.size())
                    return false;


                // Copy the Clipped Polygon Into the Pipeline Trace

                std::vector<PipelineClipVertex> polygon;
                polygon.reserve(count);

                for (std::size_t i = 0; i < count; ++i) {
                    const ClipVertex& vertex = clipped[i];

                    if (!isFinite(vertex.position) || !isFinite(vertex.uv))
                        return false;

                    PipelineClipVertex captured{};

                    captured.position = vertex.position;
                    captured.uv = vertex.uv;
                    captured.color = vertex.color;

                    polygon.push_back(captured);
                }


                // Commit Only After the Full Clipping Result Is Valid

                result.clippedPolygon = std::move(polygon);


                // Invalidate All Later Pipeline Stages

                result.ndcPolygon.reset();
                result.screenPolygon.reset();
                result.pixelSample.reset();

                return true;
            }
    };
}
