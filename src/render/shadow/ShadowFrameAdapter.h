#pragma once

#include <vector>

#include "render/frame/RenderFrame.h"
#include "render/shadow/ShadowMapping.h"


namespace ve
{
    class ShadowFrameAdapter {
        public:
            static std::vector<ShadowLightInput> lights( const RenderFrame& frame ) {
                std::vector<ShadowLightInput> result;
                result.reserve(frame.lighting.sceneLights.size());

                for (const SceneLightData& light : frame.lighting.sceneLights) {
                    ShadowLightInput input{};
                    input.objectId = light.objectId;
                    input.type = static_cast<ShadowLightKind>(light.type);
                    input.position = light.position;
                    input.emissionDirection = light.emissionDirection;
                    input.range = light.range;
                    input.outerConeDegrees = light.outerConeDegrees;
                    input.castsShadows = light.castsShadows;
                    result.push_back(input);
                }
                return result;
            }

            static std::vector<ShadowDrawInput> draws( const RenderFrame& frame ) {
                std::vector<ShadowDrawInput> result;
                result.reserve(frame.draws.size());
                for (const RenderDraw& draw : frame.draws) {
                    if (draw.mesh != nullptr)
                        result.push_back(ShadowDrawInput{ draw.mesh, draw.model });
                }
                return result;
            }

            static ShadowFrame build( const RenderFrame& frame, const ShadowSettings& settings = {} ) {
                return ShadowMapping::build(lights(frame), draws(frame), frame.cameraPosition, settings);
            }
    };
}
