#include "render/material/PbrSurfaceSampler.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "assets/texture/Texture2D.h"
#include "render/color/ColorSpace.h"


namespace ve
{
    TangentFrame tangentFrameForTriangle( const Vec3& p0, const Vec3& p1, const Vec3& p2,
                                          const Vec2& uv0, const Vec2& uv1, const Vec2& uv2,
                                          const Vec3& worldNormal ) {
        const Vec3 edge1 = p1 - p0;
        const Vec3 edge2 = p2 - p0;
        const Vec2 duv1 = uv1 - uv0;
        const Vec2 duv2 = uv2 - uv0;
        const float determinant = duv1.x * duv2.y - duv1.y * duv2.x;
        if (!std::isfinite(determinant) || std::fabs(determinant) <= 1.0e-8f)
            return TangentFrame{};

        const float inverse = 1.0f / determinant;
        const Vec3 dpdu = (edge1 * duv2.y - edge2 * duv1.y) * inverse;
        const Vec3 dpdv = (edge2 * duv1.x - edge1 * duv2.x) * inverse;
        const Vec3 n = worldNormal.normalized();
        const Vec3 tangent = dpdu - n * dot(n, dpdu);
        if (tangent.lengthSquared() <= 1.0e-12f || n.lengthSquared() <= 1.0e-12f)
            return TangentFrame{};

        const Vec3 t = tangent.normalized();
        const Vec3 bAxis = cross(n, t);
        const float handedness = dot(bAxis, dpdv) < 0.0f ? -1.0f : 1.0f;
        return TangentFrame{ t, bAxis * handedness, true };
    }


    PbrSurface samplePbrSurface( const MaterialData& material, const Vec2& uv,
                                  const Vec3& normal, const TangentFrame& tangentFrame ) {
        if (material.texture == nullptr)
            throw std::invalid_argument("PBR Material requires a BaseColor texture");
        if (material.texture->colorSpace() != TextureColorSpace::SRGB)
            throw std::invalid_argument("PBR BaseColor must use sRGB colorspace");
        for (const Texture2D* map : { material.normalMap, material.roughnessMap,
                                      material.metallicMap, material.aoMap }) {
            if (map != nullptr && map->colorSpace() != TextureColorSpace::Linear)
                throw std::invalid_argument("PBR data maps must use Linear colorspace");
        }

        const Color32 pixel = material.texture->sampleNearest(uv);
        PbrSurface result{};
        result.baseColor = decodeSrgb(pixel) * material.diffuseTint;
        result.worldNormal = normal.normalized();
        result.geometricNormal = result.worldNormal;
        result.roughness = material.roughnessFactor;
        result.metallic = material.metallicFactor;
        result.ao = 1.0f;

        if (material.roughnessMap != nullptr) {
            const Color32 channel = material.roughnessMap->sampleNearest(uv);
            result.roughness *= static_cast<float>(getRed(channel)) / 255.0f;
        }
        if (material.metallicMap != nullptr) {
            const Color32 channel = material.metallicMap->sampleNearest(uv);
            result.metallic *= static_cast<float>(getRed(channel)) / 255.0f;
        }
        if (material.aoMap != nullptr) {
            const Color32 channel = material.aoMap->sampleNearest(uv);
            const float ao = static_cast<float>(getRed(channel)) / 255.0f;
            result.ao = 1.0f - material.aoStrength * (1.0f - ao);
        }
        if (material.normalMap != nullptr && tangentFrame.valid) {
            const Color32 channel = material.normalMap->sampleNearest(uv);
            const float x = (static_cast<float>(getRed(channel)) / 255.0f * 2.0f - 1.0f) *
                            material.normalStrength;
            const float y = (static_cast<float>(getGreen(channel)) / 255.0f * 2.0f - 1.0f) *
                            material.normalStrength;
            const float z = static_cast<float>(getBlue(channel)) / 255.0f * 2.0f - 1.0f;
            const Vec3 mapped = tangentFrame.tangent * x + tangentFrame.bitangent * y + normal.normalized() * z;
            if (mapped.lengthSquared() > 1.0e-12f)
                result.worldNormal = mapped.normalized();
        }
        result.roughness = std::clamp(result.roughness, 0.045f, 1.0f);
        result.metallic = std::clamp(result.metallic, 0.0f, 1.0f);
        result.ao = std::clamp(result.ao, 0.0f, 1.0f);
        validatePbrSurface(result);
        return result;
    }
}
