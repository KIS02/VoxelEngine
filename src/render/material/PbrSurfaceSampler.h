#pragma once

#include "assets/material/MaterialData.h"
#include "math/Vec2.h"
#include "math/Vec3.h"
#include "render/lighting/pbr/PbrLighting.h"


namespace ve
{
    struct TangentFrame
    {
        Vec3 tangent{ 1.0f, 0.0f, 0.0f };
        Vec3 bitangent{ 0.0f, 1.0f, 0.0f };
        bool valid = false;
    };


    TangentFrame tangentFrameForTriangle( const Vec3& p0, const Vec3& p1, const Vec3& p2,
                                          const Vec2& uv0, const Vec2& uv1, const Vec2& uv2,
                                          const Vec3& worldNormal );

    PbrSurface samplePbrSurface( const MaterialData& material, const Vec2& uv,
                                  const Vec3& normal, const TangentFrame& tangentFrame );
}
