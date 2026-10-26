#pragma once

#include "math/Color.h"
#include "math/Vec3.h"


namespace ve
{
    struct DirectionalLight;
    struct MaterialData;


    class SoftwareFragmentShading {
        public:
            static Color32 shade( Color32 sampledColor, const Vec3& normal,
                                  const Vec3& worldPosition, const Vec3* cameraPosition,
                                  const DirectionalLight* light, const MaterialData* material );
    };
}
