#include "render/lighting/Lighting.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "render/color/ColorSpace.h"
#include "assets/material/MaterialValidation.h"


namespace ve
{
    namespace
    {
        void validateColor( const LinearRGB& color, const char* label ) {
            const auto valid = []( float value ) {
                return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
            };

            if (!valid(color.r) || !valid(color.g) || !valid(color.b))
                throw std::invalid_argument(label);
        }


        float diffuseFactor( const Vec3& normal, const DirectionalLight& light ) {
            return std::max(0.0f, dot(normal.normalized(), light.directionToLight.normalized()));
        }


        LinearRGB diffuseAndAmbient( Color32 baseColor, const Vec3& normal,
                                     const DirectionalLight& light, const LinearRGB& diffuseTint ) {
            const LinearRGB albedo = decodeSrgb(baseColor) * diffuseTint;
            const float lambert = diffuseFactor(normal, light);

            return albedo * light.ambient +
                   (albedo * light.color) * (light.diffuse * lambert);
        }


        float calculateSpecularIntensity( const Vec3& normal, const Vec3& worldPosition,
                                          const Vec3& cameraPosition, const DirectionalLight& light,
                                          const MaterialData& material ) {
            if (material.specularStrength == 0.0f)
                return 0.0f;

            const Vec3 n = normal.normalized();
            const Vec3 l = light.directionToLight.normalized();

            if (dot(n, l) <= 0.0f)
                return 0.0f;

            const Vec3 toCamera = cameraPosition - worldPosition;

            if (toCamera.lengthSquared() <= 1.0e-12f)
                return 0.0f;

            const Vec3 halfway = l + toCamera.normalized();

            if (halfway.lengthSquared() <= 1.0e-12f)
                return 0.0f;

            const float alignment = std::max(0.0f, dot(n, halfway.normalized()));
            return material.specularStrength * std::pow(alignment, material.shininess);
        }
    }


    void validateMaterialLighting( float specularStrength, float shininess ) {
        if (!std::isfinite(specularStrength) || specularStrength < 0.0f ||
            specularStrength > 1.0f || !std::isfinite(shininess) || shininess <= 0.0f)
            throw std::invalid_argument("Invalid Material specular properties");
    }


    void validateMaterialLighting( const MaterialData& material ) {
        validateMaterialProperties(material);
    }


    void validateDirectionalLighting( const DirectionalLight& light ) {
        if (!std::isfinite(light.ambient) || light.ambient < 0.0f ||
            !std::isfinite(light.diffuse) || light.diffuse < 0.0f ||
            !std::isfinite(light.directionToLight.x) ||
            !std::isfinite(light.directionToLight.y) ||
            !std::isfinite(light.directionToLight.z) ||
            light.directionToLight.lengthSquared() <= 1.0e-12f)
            throw std::invalid_argument("Invalid Directional Light");

        validateColor(light.color, "Invalid light RGB color");
    }


    Vec3 transformNormal( const NormalMatrix3& matrix, const Vec3& localNormal ) {
        return Vec3{
            matrix.m[0][0] * localNormal.x + matrix.m[0][1] * localNormal.y + matrix.m[0][2] * localNormal.z,
            matrix.m[1][0] * localNormal.x + matrix.m[1][1] * localNormal.y + matrix.m[1][2] * localNormal.z,
            matrix.m[2][0] * localNormal.x + matrix.m[2][1] * localNormal.y + matrix.m[2][2] * localNormal.z
        };
    }


    float blinnPhongSpecular( const Vec3& normal, const Vec3& worldPosition,
                              const Vec3& cameraPosition, const DirectionalLight& light,
                              const MaterialData& material ) {
        validateMaterialLighting(material);
        validateDirectionalLighting(light);
        return calculateSpecularIntensity(normal, worldPosition, cameraPosition, light, material);
    }


    Color32 shadeColorLambert( Color32 baseColor, const Vec3& worldNormal, const DirectionalLight& light ) {
        validateDirectionalLighting(light);

        const LinearRGB color = diffuseAndAmbient(
            baseColor, worldNormal, light, LinearRGB{ 1.0f, 1.0f, 1.0f }
        );

        return encodeSrgb(color, getAlpha(baseColor));
    }


    Color32 shadeColorBlinnPhong( Color32 baseColor, const Vec3& normal,
                                  const Vec3& worldPosition, const Vec3& cameraPosition,
                                  const DirectionalLight& light, const MaterialData& material ) {
        validateMaterialLighting(material);
        validateDirectionalLighting(light);

        const LinearRGB ambientDiffuse = diffuseAndAmbient(baseColor, normal, light, material.diffuseTint);
        const float strength = calculateSpecularIntensity(normal, worldPosition, cameraPosition, light, material);
        const LinearRGB specular = (material.specularColor * light.color) * strength;

        return encodeSrgb(ambientDiffuse + specular, getAlpha(baseColor));
    }

    void validateSceneLights( const LightingSettings& settings ) {
        if (settings.sceneLights.size() > MaxSceneLights)
            throw std::invalid_argument("Too many Scene Lights for the OpenGL 3.3 pipeline");

        for (const SceneLightData& light : settings.sceneLights) {
            const auto finite = []( float v ) { return std::isfinite(v); };
            const auto finiteVec = [&finite]( const Vec3& v ) {
                return finite(v.x) && finite(v.y) && finite(v.z);
            };

            if (light.type != LightType::Directional && light.type != LightType::Point &&
                light.type != LightType::Spot)
                throw std::invalid_argument("Invalid Scene Light type");

            validateColor(light.color, "Invalid Scene Light color");

            if (!finiteVec(light.position) || !finiteVec(light.emissionDirection) ||
                !finiteVec(light.directionToLight) || !finite(light.intensity) ||
                light.intensity < 0.0f || !finite(light.ambient) || light.ambient < 0.0f ||
                light.ambient > 1.0f || !finite(light.diffuse) || light.diffuse < 0.0f ||
                light.diffuse > 1.0f)
                throw std::invalid_argument("Invalid Scene Light settings");

            if (light.type == LightType::Directional &&
                light.directionToLight.lengthSquared() <= 1.0e-12f)
                throw std::invalid_argument("Directional Light direction is zero");

            if (light.type != LightType::Directional &&
                (!finite(light.range) || light.range <= 0.0f))
                throw std::invalid_argument("Point/Spot Light range must be positive");

            if (light.type == LightType::Spot) {
                if (light.emissionDirection.lengthSquared() <= 1.0e-12f ||
                    !finite(light.innerConeDegrees) || !finite(light.outerConeDegrees) ||
                    light.innerConeDegrees < 0.0f || light.outerConeDegrees >= 90.0f ||
                    light.innerConeDegrees >= light.outerConeDegrees)
                    throw std::invalid_argument("Spot Light cone configuration is invalid");
            }
        }
    }


    LinearRGB shadeLinearMultiLightValidated( Color32 baseColor, const Vec3& normal,
                                              const Vec3& worldPosition, const Vec3& cameraPosition,
                                              const LightingSettings& settings, const MaterialData& material,
                                              const ShadowVisibility* shadow ) {

        // Direct legacy calls and old GPU parity tests keep the Phase 10.7 contract.
        if (settings.sceneLights.empty()) {
            if (settings.directional.ambient == 0.0f && settings.directional.diffuse == 0.0f)
                return LinearRGB{};

            validateDirectionalLighting(settings.directional);
            const LinearRGB ambientDiffuse = diffuseAndAmbient(baseColor, normal,
                                                                settings.directional, material.diffuseTint);
            const float strength = calculateSpecularIntensity(normal, worldPosition, cameraPosition,
                                                              settings.directional, material);
            const LinearRGB specular = (material.specularColor * settings.directional.color) * strength;
            return ambientDiffuse + specular;
        }

        const LinearRGB albedo = decodeSrgb(baseColor) * material.diffuseTint;
        const Vec3 n = normal.normalized();
        const Vec3 cameraVector = cameraPosition - worldPosition;
        const bool hasView = cameraVector.lengthSquared() > 1.0e-12f;
        const Vec3 view = hasView ? cameraVector.normalized() : Vec3{};
        LinearRGB result{};

        constexpr float Radians = 3.14159265358979323846f / 180.0f;

        for (const SceneLightData& light : settings.sceneLights) {
            if (light.intensity <= 0.0f)
                continue;

            Vec3 toLight = light.directionToLight.normalized();
            float falloff = 1.0f;

            if (light.type != LightType::Directional) {
                const Vec3 offset = light.position - worldPosition;
                const float distanceSquared = offset.lengthSquared();
                const float distance = std::sqrt(distanceSquared);

                if (distance >= light.range)
                    continue;

                toLight = distanceSquared > 1.0e-12f ? offset / distance : Vec3{};
                const float t = distance / light.range;
                const float envelope = 1.0f - t * t;
                falloff = envelope * envelope / (1.0f + distanceSquared);

                if (light.type == LightType::Spot) {
                    const Vec3 emission = light.emissionDirection.normalized();
                    const float cosTheta = dot(emission, -toLight);
                    const float innerCos = std::cos(light.innerConeDegrees * Radians);
                    const float outerCos = std::cos(light.outerConeDegrees * Radians);
                    const float cone = std::clamp((cosTheta - outerCos) /
                                                  (innerCos - outerCos), 0.0f, 1.0f);
                    falloff *= cone;
                }
            }

            const float weight = falloff * light.intensity;
            if (weight <= 0.0f)
                continue;

            // Shadowing blocks direct diffuse/specular, never ambient contribution.
            const float visibility = shadow != nullptr && light.castsShadows
                ? shadow->visibility(light.objectId, worldPosition, n, toLight) : 1.0f;
            result = result + albedo * (light.ambient * weight);
            const float facing = std::max(0.0f, dot(n, toLight));
            result = result + (albedo * light.color) * (light.diffuse * facing * weight * visibility);

            if (facing > 0.0f && material.specularStrength > 0.0f && hasView) {
                const Vec3 halfway = toLight + view;
                if (halfway.lengthSquared() > 1.0e-12f) {
                    const float alignment = std::max(0.0f, dot(n, halfway.normalized()));
                    const float strength = material.specularStrength *
                                           std::pow(alignment, material.shininess) * weight * visibility;
                    result = result + (material.specularColor * light.color) * strength;
                }
            }
        }

        return result;
    }

    LinearRGB shadeLinearMultiLight( Color32 baseColor, const Vec3& normal,
                                    const Vec3& worldPosition, const Vec3& cameraPosition,
                                    const LightingSettings& settings, const MaterialData& material,
                                    const ShadowVisibility* shadow ) {
        validateMaterialLighting(material);
        validateSceneLights(settings);
        return shadeLinearMultiLightValidated(baseColor, normal, worldPosition, cameraPosition,
                                              settings, material, shadow);
    }

    Color32 shadeColorMultiLight( Color32 baseColor, const Vec3& normal,
                                  const Vec3& worldPosition, const Vec3& cameraPosition,
                                  const LightingSettings& settings, const MaterialData& material,
                                  const ShadowVisibility* shadow ) {
        if (settings.sceneLights.empty()) {
            if (settings.directional.ambient == 0.0f && settings.directional.diffuse == 0.0f)
                return encodeSrgb(LinearRGB{}, getAlpha(baseColor));

            return shadeColorBlinnPhong(baseColor, normal, worldPosition, cameraPosition,
                                       settings.directional, material);
        }

        return encodeSrgb(shadeLinearMultiLight(baseColor, normal, worldPosition, cameraPosition,
                                               settings, material, shadow), getAlpha(baseColor));
    }

}
