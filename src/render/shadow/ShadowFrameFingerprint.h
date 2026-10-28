#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

#include "render/frame/RenderFrame.h"
#include "render/shadow/ShadowMapping.h"


namespace ve
{
    // Fingerprints only the inputs used to construct shadow depth maps.
    // Mesh revisions, models, caster transforms, and camera position invalidate the cache.
    class ShadowFrameFingerprint {
        private:
            static constexpr std::uint64_t Offset = 14695981039346656037ull;
            static constexpr std::uint64_t Prime = 1099511628211ull;

            static void add( std::uint64_t& value, std::uint64_t input ) {
                for (int i = 0; i < 8; ++i) {
                    value ^= static_cast<std::uint8_t>(input & 0xffu);
                    value *= Prime;
                    input >>= 8;
                }
            }

            static void addFloat( std::uint64_t& value, float input ) {
                std::uint32_t bits = 0;
                std::memcpy(&bits, &input, sizeof(bits));
                add(value, bits);
            }

            static void addVec3( std::uint64_t& value, const Vec3& input ) {
                addFloat(value, input.x);
                addFloat(value, input.y);
                addFloat(value, input.z);
            }

        public:
            static std::uint64_t calculate( const RenderFrame& frame, const ShadowSettings& settings ) {
                std::uint64_t value = Offset;

                add(value, static_cast<std::uint64_t>(settings.resolution));
                add(value, static_cast<std::uint64_t>(settings.pcfRadius));
                addFloat(value, settings.nearPlane);
                addFloat(value, settings.directionalHalfExtent);
                addFloat(value, settings.directionalDistance);
                addFloat(value, settings.bias);
                addFloat(value, settings.slopeBias);
                addFloat(value, settings.normalOffset);
                addVec3(value, frame.cameraPosition);

                add(value, frame.lighting.sceneLights.size());
                for (const SceneLightData& light : frame.lighting.sceneLights) {
                    add(value, light.objectId);
                    add(value, static_cast<std::uint64_t>(light.type));
                    add(value, light.castsShadows ? 1ull : 0ull);
                    addVec3(value, light.position);
                    addVec3(value, light.emissionDirection);
                    addFloat(value, light.range);
                    addFloat(value, light.outerConeDegrees);
                }

                add(value, frame.draws.size());
                for (const RenderDraw& draw : frame.draws) {
                    add(value, draw.meshId);
                    add(value, draw.meshRevision);
                    add(value, static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(draw.mesh)));
                    for (int row = 0; row < 4; ++row) {
                        for (int column = 0; column < 4; ++column)
                            addFloat(value, draw.model(row, column));
                    }
                }

                return value;
            }
    };
}
