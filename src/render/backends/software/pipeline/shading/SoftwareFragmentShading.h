#pragma once

#include "math/Color.h"
#include "math/LinearRGB.h"
#include "math/Vec3.h"


namespace ve
{
    struct DirectionalLight;
    struct MaterialData;
    struct LightingSettings;
    class ShadowVisibility;


    class SoftwareFragmentShading {
        public:
            static LinearRGB shadeLinear( Color32 sampledColor, const Vec3& normal,
                                          const Vec3& worldPosition, const Vec3* cameraPosition,
                                          const DirectionalLight* light, const MaterialData* material,
                                          const LightingSettings* sceneLighting,
                                          const ShadowVisibility* shadow );

            static Color32 shade( Color32 sampledColor, const Vec3& normal,
                                  const Vec3& worldPosition, const Vec3* cameraPosition,
                                  const DirectionalLight* light, const MaterialData* material,
                                  const LightingSettings* sceneLighting = nullptr,
                                  const ShadowVisibility* shadow = nullptr );
    };
}
