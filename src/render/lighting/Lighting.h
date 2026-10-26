#pragma once

#include "assets/material/MaterialData.h"
#include "math/Color.h"
#include "math/LinearRGB.h"
#include "math/Vec3.h"
#include "render/lighting/DirectionalLight.h"
#include "render/lighting/NormalMatrix3.h"


namespace ve
{
    struct LightingSettings
    {
        DirectionalLight directional{ Vec3{ -0.45f, 1.0f, 0.35f }, 0.25f, 0.75f };
    };


    void validateMaterialLighting( float specularStrength, float shininess );
    void validateMaterialLighting( const MaterialData& material );
    void validateDirectionalLighting( const DirectionalLight& light );

    Vec3 transformNormal( const NormalMatrix3& matrix, const Vec3& localNormal );

    float blinnPhongSpecular( const Vec3& normal, const Vec3& worldPosition,
                              const Vec3& cameraPosition, const DirectionalLight& light,
                              const MaterialData& material );

    Color32 shadeColorLambert( Color32 baseColor, const Vec3& worldNormal, const DirectionalLight& light );

    Color32 shadeColorBlinnPhong( Color32 baseColor, const Vec3& normal,
                                  const Vec3& worldPosition, const Vec3& cameraPosition,
                                  const DirectionalLight& light, const MaterialData& material );
}