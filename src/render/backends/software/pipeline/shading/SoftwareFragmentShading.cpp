#include "render/backends/software/pipeline/shading/SoftwareFragmentShading.h"

#include "render/lighting/Lighting.h"
#include "render/color/ColorSpace.h"


namespace ve
{
    LinearRGB SoftwareFragmentShading::shadeLinear( Color32 sampledColor, const Vec3& normal,
                                                    const Vec3& worldPosition, const Vec3* cameraPosition,
                                                    const DirectionalLight* light, const MaterialData* material,
                                                    const LightingSettings* sceneLighting,
                                                    const ShadowVisibility* shadow ) {
        if (sceneLighting != nullptr && material != nullptr && cameraPosition != nullptr)
            return shadeLinearMultiLight(sampledColor, normal, worldPosition,
                                         *cameraPosition, *sceneLighting, *material, shadow);

        // Legacy raster callers without full Light settings remain SDR-compatible.
        return decodeSrgb(shade(sampledColor, normal, worldPosition, cameraPosition,
                                light, material, sceneLighting, shadow));
    }


    Color32 SoftwareFragmentShading::shade( Color32 sampledColor, const Vec3& normal,
                                            const Vec3& worldPosition, const Vec3* cameraPosition,
                                            const DirectionalLight* light, const MaterialData* material,
                                            const LightingSettings* sceneLighting, const ShadowVisibility* shadow ) {
        if (sceneLighting != nullptr && material != nullptr && cameraPosition != nullptr &&
            (!sceneLighting->sceneLights.empty() ||
             (sceneLighting->directional.ambient == 0.0f && sceneLighting->directional.diffuse == 0.0f)))
            return shadeColorMultiLight(sampledColor, normal, worldPosition,
                                        *cameraPosition, *sceneLighting, *material, shadow);

        if (light == nullptr)
            return sampledColor;

        if (material != nullptr && cameraPosition != nullptr)
            return shadeColorBlinnPhong(sampledColor, normal, worldPosition,
                                       *cameraPosition, *light, *material);

        return shadeColorLambert(sampledColor, normal, *light);
    }
}
