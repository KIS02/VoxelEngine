#pragma once

#include "math/LinearRGB.h"
#include "math/Vec3.h"
#include "render/lighting/Lighting.h"


namespace ve
{
    struct PbrSurface
    {
        LinearRGB baseColor{ 1.0f, 1.0f, 1.0f };
        Vec3 worldNormal{ 0.0f, 1.0f, 0.0f };
        Vec3 geometricNormal{ 0.0f, 1.0f, 0.0f };
        float metallic = 0.0f;
        float roughness = 0.5f;
        float ao = 1.0f;
    };


    void validatePbrSurface( const PbrSurface& surface );

    LinearRGB shadePbrDirect( const PbrSurface& surface, const Vec3& worldPosition,
                              const Vec3& cameraPosition, const LightingSettings& lighting,
                              const ShadowVisibility* shadow = nullptr );
}
