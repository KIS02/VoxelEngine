
#pragma once

#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <utility>

#include "render/MeshData.h"
#include "render/RenderFrame.h"

#include "render/pipeline/PipelineTriangleSelector.h"
#include "render/pipeline/PipelineTransformTracer.h"
#include "render/pipeline/PipelineClippingTracer.h"
#include "render/pipeline/PipelineViewportTracer.h"
#include "render/pipeline/PipelineRasterizationTracer.h"


namespace ve
{
    class PipelineTrianglePicker {
        public:
            static std::optional<PipelineTrace> pick( const RenderFrame& frame,
                                                       int framebufferWidth, int framebufferHeight,
                                                       int pixelX, int pixelY ) {
                if (framebufferWidth <= 0 || framebufferHeight <= 0)
                    return std::nullopt;

                if (pixelX < 0 || pixelY < 0 ||
                    pixelX >= framebufferWidth || pixelY >= framebufferHeight)
                    return std::nullopt;


                // Keep the Closest Rasterized Triangle

                std::optional<PipelineTrace> closestTrace{};
                float closestDepth = std::numeric_limits<float>::infinity();

                PipelineTriangleSelector selector{};


                // Iterate Through All Draws

                for (std::size_t drawIndex = 0; drawIndex < frame.draws.size(); ++drawIndex) {
                    const RenderDraw& draw = frame.draws[drawIndex];

                    if (draw.mesh == nullptr)
                        continue;

                    const std::size_t triangleCount = draw.mesh->indexCount() / 3;


                    // Analyze Each Triangle Only During Picking

                    for (std::size_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex) {
                        if (!selector.select(frame, framebufferWidth, framebufferHeight,
                                             drawIndex, triangleIndex))
                            continue;


                        // Create an Independent Candidate Trace

                        PipelineTrace candidate{};
                        candidate.triangle = selector.snapshot();


                        // Reuse the Existing CPU Reference Pipeline

                        if (!PipelineTransformTracer::trace(candidate))
                            continue;

                        if (!PipelineClippingTracer::trace(candidate))
                            continue;

                        if (!PipelineViewportTracer::trace(candidate))
                            continue;

                        if (!PipelineRasterizationTracer::trace(candidate, pixelX, pixelY))
                            continue;


                        // Reject Triangles That Do Not Cover the Pixel

                        if (!candidate.pixelSample.has_value() || !candidate.pixelSample->covered)
                            continue;

                        const float depth = candidate.pixelSample->incomingDepth;


                        // NDC Depth Can Be Negative.
                        // Clipping Already Validated the Visible Depth Range.

                        if (!std::isfinite(depth))
                            continue;


                        // Select the Closest Candidate

                        if (depth >= closestDepth)
                            continue;

                        closestDepth = depth;
                        closestTrace = std::move(candidate);
                    }
                }

                return closestTrace;
            }
    };
}
