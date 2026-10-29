#include "render/lighting/pbr/PbrLighting.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>


namespace ve
{
    namespace
    {
        constexpr float Pi = 3.14159265358979323846f;

        bool validUnit( float v ) {
            return std::isfinite(v) && v >= 0.0f && v <= 1.0f;
        }

        bool finite( const Vec3& v ) {
            return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
        }

        LinearRGB multiply( const LinearRGB& a, const LinearRGB& b ) {
            return a * b;
        }

        LinearRGB mix( const LinearRGB& a, const LinearRGB& b, float f ) {
            return a * (1.0f - f) + b * f;
        }

        LinearRGB fresnelSchlick( float hDotV, const LinearRGB& f0 ) {
            const float f = std::pow(1.0f - std::clamp(hDotV, 0.0f, 1.0f), 5.0f);
            return f0 + (LinearRGB{ 1.0f, 1.0f, 1.0f } + f0 * -1.0f) * f;
        }

        float distributionGgx( float nDotH, float roughness ) {
            const float alpha = roughness * roughness;
            const float a2 = alpha * alpha;
            const float d = nDotH * nDotH * (a2 - 1.0f) + 1.0f;
            return a2 / std::max(Pi * d * d, 1.0e-8f);
        }

        float geometrySchlickGgx( float nDotX, float roughness ) {
            const float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
            return nDotX / std::max(nDotX * (1.0f - k) + k, 1.0e-8f);
        }

        float geometrySmith( float nDotV, float nDotL, float roughness ) {
            return geometrySchlickGgx(nDotV, roughness) * geometrySchlickGgx(nDotL, roughness);
        }

        float lightFalloff( const SceneLightData& light, const Vec3& worldPosition, Vec3& direction ) {
            direction = light.directionToLight.normalized();
            if (light.type == LightType::Directional)
                return 1.0f;

            const Vec3 delta = light.position - worldPosition;
            const float distanceSquared = delta.lengthSquared();
            const float distance = std::sqrt(distanceSquared);
            if (distance >= light.range)
                return 0.0f;

            direction = distanceSquared > 1.0e-12f ? delta / distance : Vec3{};
            const float t = distance / light.range;
            const float envelope = 1.0f - t * t;
            float attenuation = envelope * envelope / (1.0f + distanceSquared);

            if (light.type == LightType::Spot) {
                const Vec3 emission = light.emissionDirection.normalized();
                const float cosTheta = dot(emission, -direction);
                const float innerCos = std::cos(light.innerConeDegrees * Pi / 180.0f);
                const float outerCos = std::cos(light.outerConeDegrees * Pi / 180.0f);
                attenuation *= std::clamp((cosTheta - outerCos) / (innerCos - outerCos), 0.0f, 1.0f);
            }
            return attenuation;
        }
    }


    void validatePbrSurface( const PbrSurface& surface ) {
        if (!validUnit(surface.metallic) || !validUnit(surface.roughness) || !validUnit(surface.ao) ||
            !validUnit(surface.baseColor.r) || !validUnit(surface.baseColor.g) ||
            !validUnit(surface.baseColor.b) || !finite(surface.worldNormal) ||
            !finite(surface.geometricNormal) || surface.geometricNormal.lengthSquared() <= 1.0e-12f ||
            surface.worldNormal.lengthSquared() <= 1.0e-12f)
            throw std::invalid_argument("Invalid PBR Surface");
    }


    LinearRGB shadePbrDirect( const PbrSurface& surface, const Vec3& worldPosition,
                              const Vec3& cameraPosition, const LightingSettings& lighting,
                              const ShadowVisibility* shadow ) {
        validatePbrSurface(surface);
        validateSceneLights(lighting);
        if (!finite(worldPosition) || !finite(cameraPosition))
            throw std::invalid_argument("Nonfinite PBR world or camera position");

        // Minimum alpha guards GGX singularities at roughness == 0.
        const float roughness = std::max(0.045f, surface.roughness);
        const Vec3 normal = surface.worldNormal.normalized();
        const Vec3 viewVector = cameraPosition - worldPosition;
        if (viewVector.lengthSquared() <= 1.0e-12f)
            return LinearRGB{};
        const Vec3 view = viewVector.normalized();
        const float nDotV = std::max(dot(normal, view), 0.0f);
        if (nDotV <= 0.0f)
            return LinearRGB{};

        const LinearRGB f0 = mix(LinearRGB{ 0.04f, 0.04f, 0.04f }, surface.baseColor, surface.metallic);
        LinearRGB result{};

        for (const SceneLightData& light : lighting.sceneLights) {
            if (light.intensity <= 0.0f)
                continue;

            Vec3 l{};
            const float attenuation = lightFalloff(light, worldPosition, l);
            if (attenuation <= 0.0f)
                continue;

            // Ambient is an approximation, not environment-map-based IBL.
            result = result + surface.baseColor * (light.ambient * light.intensity * attenuation * surface.ao *
                                                    (1.0f - surface.metallic));

            const float nDotL = std::max(dot(normal, l), 0.0f);
            if (nDotL <= 0.0f)
                continue;
            const Vec3 halfVector = l + view;
            if (halfVector.lengthSquared() <= 1.0e-12f)
                continue;

            const Vec3 h = halfVector.normalized();
            const float nDotH = std::max(dot(normal, h), 0.0f);
            const float hDotV = std::max(dot(h, view), 0.0f);
            const LinearRGB fresnel = fresnelSchlick(hDotV, f0);
            const float d = distributionGgx(nDotH, roughness);
            const float g = geometrySmith(nDotV, nDotL, roughness);
            const LinearRGB specular = fresnel * (d * g / std::max(4.0f * nDotV * nDotL, 1.0e-4f));
            const LinearRGB kd = (LinearRGB{ 1.0f, 1.0f, 1.0f } + fresnel * -1.0f) *
                                 (1.0f - surface.metallic);
            const LinearRGB diffuse = multiply(kd, surface.baseColor) * (1.0f / Pi);
            const float visibility = shadow != nullptr && light.castsShadows
                ? shadow->visibility(light.objectId, worldPosition, surface.geometricNormal.normalized(), l) : 1.0f;
            const LinearRGB radiance = light.color * (light.intensity * attenuation * light.diffuse);
            result = result + multiply(diffuse + specular, radiance) * (nDotL * visibility);
        }
        return result;
    }
}
