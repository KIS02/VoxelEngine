#pragma once

#include "assets/material/MaterialData.h"
#include "math/Color.h"
#include "math/LinearRGB.h"
#include "math/Vec3.h"
#include "render/lighting/DirectionalLight.h"
#include "render/lighting/NormalMatrix3.h"
#include "scene/lighting/SceneLightData.h"
#include "render/shadow/ShadowVisibility.h"

#include <cstddef>
#include <vector>


namespace ve
{
    struct LightingSettings
    {
        DirectionalLight directional{ Vec3{ -0.45f, 1.0f, 0.35f }, 0.25f, 0.75f };
        std::vector<SceneLightData> sceneLights{};
    };


    void validateMaterialLighting( float specularStrength, float shininess );
    inline constexpr std::size_t MaxSceneLights = 16;


    void validateSceneLights( const LightingSettings& settings );

    LinearRGB shadeLinearMultiLight( Color32 baseColor, const Vec3& normal,
                                     const Vec3& worldPosition, const Vec3& cameraPosition,
                                     const LightingSettings& settings, const MaterialData& material,
                                     const ShadowVisibility* shadow = nullptr );

    // Internal fast path. Call only after validateMaterialLighting() and validateSceneLights().
    LinearRGB shadeLinearMultiLightValidated( Color32 baseColor, const Vec3& normal,
                                              const Vec3& worldPosition, const Vec3& cameraPosition,
                                              const LightingSettings& settings, const MaterialData& material,
                                              const ShadowVisibility* shadow = nullptr );

    Color32 shadeColorMultiLight( Color32 baseColor, const Vec3& normal,
                                  const Vec3& worldPosition, const Vec3& cameraPosition,
                                  const LightingSettings& settings, const MaterialData& material,
                                  const ShadowVisibility* shadow = nullptr );

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
