#include "render/backends/software/pipeline/shading/SoftwareFragmentShading.h"

#include "render/lighting/Lighting.h"


namespace ve
{
    Color32 SoftwareFragmentShading::shade( Color32 sampledColor, const Vec3& normal,
                                            const Vec3& worldPosition, const Vec3* cameraPosition,
                                            const DirectionalLight* light, const MaterialData* material ) {
        if (light == nullptr)
            return sampledColor;

        if (material != nullptr && cameraPosition != nullptr)
            return shadeColorBlinnPhong(sampledColor, normal, worldPosition,
                                       *cameraPosition, *light, *material);

        return shadeColorLambert(sampledColor, normal, *light);
    }
}
