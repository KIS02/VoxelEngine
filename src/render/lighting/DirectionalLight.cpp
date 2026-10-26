#include "render/lighting/DirectionalLight.h"

#include <algorithm>


namespace ve
{
    Vec3 triangleNormal( const Vec3& a, const Vec3& b, const Vec3& c ) {
        return cross(b - a, c - a).normalized();
    }


    float diffuseFactor( const Vec3& normal, const Vec3& directionToLight ) {
        const Vec3 n = normal.normalized();
        const Vec3 l = directionToLight.normalized();

        return std::max(0.0f, dot(n, l));
    }


    float lightingFactor( const Vec3& normal, const DirectionalLight& light ) {
        const float intensity = light.ambient + light.diffuse * diffuseFactor(normal, light.directionToLight);

        return std::clamp(intensity, 0.0f, 1.0f);
    }
}