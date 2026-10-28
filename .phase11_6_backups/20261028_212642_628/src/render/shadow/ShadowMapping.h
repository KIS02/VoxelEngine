#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "math/Mat4.h"
#include "math/Vec3.h"
#include "render/shadow/ShadowVisibility.h"


namespace ve
{
    enum class ShadowLightKind : std::uint8_t {
        Directional, Point, Spot
    };


    struct ShadowLightInput
    {
        std::uint64_t objectId = 0;
        ShadowLightKind type = ShadowLightKind::Directional;
        Vec3 position{};
        Vec3 emissionDirection{ 0.0f, -1.0f, 0.0f };
        float range = 20.0f;
        float outerConeDegrees = 30.0f;
        bool castsShadows = false;
    };


    struct ShadowDrawInput
    {
        const class MeshData* mesh = nullptr;
        Mat4 model = Mat4::identity();
    };


    struct ShadowSettings
    {
        int resolution = 256;
        float nearPlane = 0.1f;
        float directionalHalfExtent = 32.0f;
        float directionalDistance = 64.0f;
        float bias = 0.002f;
        float slopeBias = 0.005f;
        int pcfRadius = 1;
    };


    struct ShadowDepthFace
    {
        Mat4 worldToClip = Mat4::identity();
        int resolution = 0;
        std::vector<float> depths{};

        float depth( int x, int y ) const;
    };


    struct ShadowLightMap
    {
        std::uint64_t objectId = 0;
        ShadowLightKind type = ShadowLightKind::Directional;
        Vec3 position{};
        Vec3 emissionDirection{ 0.0f, -1.0f, 0.0f };
        float range = 20.0f;
        std::vector<ShadowDepthFace> faces{};
    };


    struct ShadowFrame : ShadowVisibility
    {
        ShadowSettings settings{};
        std::vector<ShadowLightMap> lights{};

        float visibility( std::uint64_t objectId, const Vec3& worldPosition,
                          const Vec3& worldNormal, const Vec3& directionToLight ) const override;
    };


    class ShadowMapping {
        public:
            static ShadowFrame prepare( const std::vector<ShadowLightInput>& lights,
                                        const Vec3& cameraPosition, const ShadowSettings& settings = {} );

            static ShadowFrame build( const std::vector<ShadowLightInput>& lights,
                                      const std::vector<ShadowDrawInput>& draws,
                                      const Vec3& cameraPosition, const ShadowSettings& settings = {} );
    };
}
