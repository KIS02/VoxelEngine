
#pragma once

#include <cmath>

#include "render/Texture2D.h"
#include "render/pipeline/PipelineTrace.h"


namespace ve
{
    class PipelineFragmentTracer {
        private:
            static bool isFinite( const Vec2& value ) {
                return std::isfinite(value.x) && std::isfinite(value.y);
            }


        public:
            static bool trace( PipelineTrace& result, float storedDepth,
                               const Texture2D& texture ) {
                if (!result.pixelSample.has_value() || !result.isConsistent())
                    return false;

                if (!std::isfinite(storedDepth))
                    return false;

                if (texture.width() <= 0 || texture.height() <= 0)
                    return false;


                // Validate the Selected Pixel

                const PipelineTriangleSnapshot& triangle = *result.triangle;
                const PipelinePixelSample& source = *result.pixelSample;

                if (source.x < 0 || source.y < 0 ||
                    source.x >= triangle.framebufferWidth ||
                    source.y >= triangle.framebufferHeight)
                    return false;

                if (source.covered) {
                    if (!std::isfinite(source.incomingDepth) || !isFinite(source.uv))
                        return false;
                }


                // Prepare a New Result Without Modifying the Original

                PipelinePixelSample sample = source;

                sample.storedDepth = storedDepth;
                sample.depthTestEvaluated = false;
                sample.depthPassed = false;
                sample.textureSampled = false;
                sample.candidateColor = makeColor(0, 0, 0);


                // Pixels Outside the Triangle Have No Fragment

                if (sample.covered) {
                    // Same GL_LESS-style Comparison as Software Rasterizer

                    sample.depthTestEvaluated = true;
                    sample.depthPassed = sample.incomingDepth < sample.storedDepth;


                    // The Software Renderer Samples Only Passing Fragments

                    if (sample.depthPassed) {
                        sample.candidateColor = texture.sampleNearest(sample.uv);
                        sample.textureSampled = true;
                    }
                }


                // Commit the Fragment Analysis

                result.pixelSample = sample;

                return true;
            }
    };
}
