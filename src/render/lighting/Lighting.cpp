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
}