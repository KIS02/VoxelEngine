#include "render/shadow/ShadowMapping.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

#include "assets/mesh/MeshData.h"
#include "math/Vec4.h"


namespace ve
{
    namespace
    {
        constexpr float Pi = 3.14159265358979323846f;
        constexpr std::size_t MaxClippedVertices = 16;

        bool finite( const Vec3& v ) {
            return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
        }

        Vec3 safeUp( const Vec3& direction ) {
            return std::fabs(direction.y) > 0.95f ? Vec3{ 0.0f, 0.0f, 1.0f }
                                                   : Vec3{ 0.0f, 1.0f, 0.0f };
        }

        Mat4 ortho( float halfExtent, float nearPlane, float farPlane ) {
            Mat4 result = Mat4::identity();
            result(0, 0) = 1.0f / halfExtent;
            result(1, 1) = 1.0f / halfExtent;
            result(2, 2) = -2.0f / (farPlane - nearPlane);
            result(2, 3) = -(farPlane + nearPlane) / (farPlane - nearPlane);
            return result;
        }

        Vec4 lerp( const Vec4& a, const Vec4& b, float t ) {
            return a + (b - a) * t;
        }

        float planeDistance( const Vec4& p, int plane ) {
            switch (plane) {
                case 0: return p.w + p.x;
                case 1: return p.w - p.x;
                case 2: return p.w + p.y;
                case 3: return p.w - p.y;
                case 4: return p.w + p.z;
                default: return p.w - p.z;
            }
        }

        std::size_t clipTriangle( const Vec4& a, const Vec4& b, const Vec4& c,
                                  std::array<Vec4, MaxClippedVertices>& output ) {
            std::array<Vec4, MaxClippedVertices> current{};
            std::array<Vec4, MaxClippedVertices> next{};
            current[0] = a;
            current[1] = b;
            current[2] = c;
            std::size_t count = 3;

            for (int plane = 0; plane < 6; ++plane) {
                if (count == 0)
                    break;

                std::size_t nextCount = 0;
                Vec4 previous = current[count - 1];
                float previousDistance = planeDistance(previous, plane);

                for (std::size_t i = 0; i < count; ++i) {
                    const Vec4& vertex = current[i];
                    const float distance = planeDistance(vertex, plane);
                    const bool wasInside = previousDistance >= 0.0f;
                    const bool inside = distance >= 0.0f;

                    if (wasInside != inside) {
                        const float t = previousDistance / (previousDistance - distance);
                        if (nextCount >= MaxClippedVertices)
                            throw std::runtime_error("Shadow clipping exceeded polygon vertex limit");
                        next[nextCount++] = lerp(previous, vertex, t);
                    }

                    if (inside) {
                        if (nextCount >= MaxClippedVertices)
                            throw std::runtime_error("Shadow clipping exceeded polygon vertex limit");
                        next[nextCount++] = vertex;
                    }

                    previous = vertex;
                    previousDistance = distance;
                }

                current.swap(next);
                count = nextCount;
            }

            for (std::size_t i = 0; i < count; ++i)
                output[i] = current[i];
            return count;
        }


        struct ScreenPoint
        {
            float x = 0.0f;
            float y = 0.0f;
            float depth = 1.0f;
        };

        ScreenPoint toScreen( const Vec4& p, int resolution ) {
            const float inverseW = 1.0f / p.w;
            return ScreenPoint{
                (p.x * inverseW * 0.5f + 0.5f) * static_cast<float>(resolution),
                (p.y * inverseW * 0.5f + 0.5f) * static_cast<float>(resolution),
                p.z * inverseW * 0.5f + 0.5f
            };
        }

        float cross2D( const ScreenPoint& a, const ScreenPoint& b, float x, float y ) {
            return (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
        }

        void rasterTriangle( ShadowDepthFace& face, const Vec4& ca, const Vec4& cb, const Vec4& cc ) {
            const ScreenPoint a = toScreen(ca, face.resolution);
            const ScreenPoint b = toScreen(cb, face.resolution);
            const ScreenPoint c = toScreen(cc, face.resolution);
            const float area = cross2D(a, b, c.x, c.y);

            if (!std::isfinite(area) || std::fabs(area) < 1.0e-8f)
                return;

            // Match the intent of glPolygonOffset(1, 1.5) in the GPU depth pass.
            const float dx = ((b.depth - a.depth) * (c.y - a.y) -
                              (c.depth - a.depth) * (b.y - a.y)) / area;
            const float dy = ((b.x - a.x) * (c.depth - a.depth) -
                              (c.x - a.x) * (b.depth - a.depth)) / area;
            const float depthOffset = std::max(std::fabs(dx), std::fabs(dy)) +
                                      1.5f / 16777216.0f;

            const float minX = std::min({ a.x, b.x, c.x });
            const float maxX = std::max({ a.x, b.x, c.x });
            const float minY = std::min({ a.y, b.y, c.y });
            const float maxY = std::max({ a.y, b.y, c.y });
            const int left = std::clamp(static_cast<int>(std::floor(minX)), 0, face.resolution - 1);
            const int right = std::clamp(static_cast<int>(std::ceil(maxX)), 0, face.resolution - 1);
            const int bottom = std::clamp(static_cast<int>(std::floor(minY)), 0, face.resolution - 1);
            const int top = std::clamp(static_cast<int>(std::ceil(maxY)), 0, face.resolution - 1);

            for (int y = bottom; y <= top; ++y) {
                for (int x = left; x <= right; ++x) {
                    const float sx = static_cast<float>(x) + 0.5f;
                    const float sy = static_cast<float>(y) + 0.5f;
                    const float w0 = cross2D(b, c, sx, sy) / area;
                    const float w1 = cross2D(c, a, sx, sy) / area;
                    const float w2 = 1.0f - w0 - w1;

                    if (w0 < -1.0e-6f || w1 < -1.0e-6f || w2 < -1.0e-6f)
                        continue;

                    const float depth = w0 * a.depth + w1 * b.depth + w2 * c.depth + depthOffset;
                    const std::size_t index = static_cast<std::size_t>(y) * face.resolution + x;
                    face.depths[index] = std::min(face.depths[index], std::clamp(depth, 0.0f, 1.0f));
                }
            }
        }

        void drawDepth( ShadowDepthFace& face, const std::vector<ShadowDrawInput>& draws ) {
            for (const ShadowDrawInput& draw : draws) {
                if (draw.mesh == nullptr)
                    throw std::invalid_argument("Shadow Draw needs a Mesh");

                const auto& vertices = draw.mesh->vertices();
                const auto& indices = draw.mesh->indices();
                if (indices.size() % 3 != 0)
                    throw std::invalid_argument("Shadow Mesh indices must be triangles");

                const Mat4 mvp = face.worldToClip * draw.model;
                for (std::size_t i = 0; i < indices.size(); i += 3) {
                    const std::uint32_t i0 = indices[i];
                    const std::uint32_t i1 = indices[i + 1];
                    const std::uint32_t i2 = indices[i + 2];
                    if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size())
                        throw std::invalid_argument("Shadow Mesh index out of range");

                    std::array<Vec4, MaxClippedVertices> polygon{};
                    const std::size_t count = clipTriangle(
                        mvp * Vec4{ vertices[i0].position, 1.0f },
                        mvp * Vec4{ vertices[i1].position, 1.0f },
                        mvp * Vec4{ vertices[i2].position, 1.0f }, polygon
                    );

                    for (std::size_t j = 1; j + 1 < count; ++j)
                        rasterTriangle(face, polygon[0], polygon[j], polygon[j + 1]);
                }
            }
        }

        ShadowDepthFace makeFace( const Mat4& matrix, int resolution,
                                  const std::vector<ShadowDrawInput>* draws ) {
            ShadowDepthFace face{};
            face.worldToClip = matrix;
            face.resolution = resolution;
            face.depths.assign(static_cast<std::size_t>(resolution) * resolution, 1.0f);
            if (draws != nullptr)
                drawDepth(face, *draws);
            return face;
        }

        int faceIndex( const Vec3& direction ) {
            const float x = std::fabs(direction.x);
            const float y = std::fabs(direction.y);
            const float z = std::fabs(direction.z);
            if (x >= y && x >= z)
                return direction.x >= 0.0f ? 0 : 1;
            if (y >= z)
                return direction.y >= 0.0f ? 2 : 3;
            return direction.z >= 0.0f ? 4 : 5;
        }

        float sampleFace( const ShadowDepthFace& face, const Vec3& position,
                          float bias, int radius ) {
            const Vec4 projected = face.worldToClip * Vec4{ position, 1.0f };
            if (projected.w <= 0.0f)
                return 1.0f;

            const float nx = projected.x / projected.w;
            const float ny = projected.y / projected.w;
            const float nz = projected.z / projected.w;
            if (nx < -1.0f || nx > 1.0f || ny < -1.0f || ny > 1.0f || nz < -1.0f || nz > 1.0f)
                return 1.0f;

            const float x = (nx * 0.5f + 0.5f) * static_cast<float>(face.resolution);
            const float y = (ny * 0.5f + 0.5f) * static_cast<float>(face.resolution);
            const float depth = nz * 0.5f + 0.5f;
            float visible = 0.0f;
            int samples = 0;

            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    const int ix = static_cast<int>(std::floor(x)) + dx;
                    const int iy = static_cast<int>(std::floor(y)) + dy;
                    if (ix < 0 || ix >= face.resolution || iy < 0 || iy >= face.resolution)
                        visible += 1.0f;
                    else
                        visible += depth - bias <= face.depth(ix, iy) ? 1.0f : 0.0f;
                    ++samples;
                }
            }
            return visible / static_cast<float>(samples);
        }
    }


    float ShadowDepthFace::depth( int x, int y ) const {
        if (x < 0 || y < 0 || x >= resolution || y >= resolution)
            throw std::out_of_range("Shadow pixel is outside the map");
        return depths[static_cast<std::size_t>(y) * resolution + x];
    }


    float ShadowFrame::visibility( std::uint64_t objectId, const Vec3& worldPosition,
                                    const Vec3& worldNormal, const Vec3& directionToLight ) const {
        for (const ShadowLightMap& light : lights) {
            if (light.objectId != objectId)
                continue;

            if (light.faces.empty())
                return 1.0f;

            const Vec3 n = worldNormal.lengthSquared() > 1.0e-12f ? worldNormal.normalized() : Vec3{};
            const Vec3 l = directionToLight.lengthSquared() > 1.0e-12f
                ? directionToLight.normalized() : Vec3{};
            const float bias = settings.bias + settings.slopeBias * (1.0f - std::max(0.0f, dot(n, l)));
            const int index = light.type == ShadowLightKind::Point
                ? faceIndex(worldPosition - light.position) : 0;
            return sampleFace(light.faces[static_cast<std::size_t>(index)], worldPosition,
                              bias, settings.pcfRadius);
        }
        return 1.0f;
    }


    namespace
    {
        ShadowFrame makeShadowFrame( const std::vector<ShadowLightInput>& lights,
                                     const std::vector<ShadowDrawInput>* draws,
                                     const Vec3& cameraPosition, const ShadowSettings& settings ) {
        if (settings.resolution < 4 || settings.resolution > 2048 ||
            !std::isfinite(settings.nearPlane) || settings.nearPlane <= 0.0f ||
            !std::isfinite(settings.directionalHalfExtent) || settings.directionalHalfExtent <= 0.0f ||
            !std::isfinite(settings.directionalDistance) || settings.directionalDistance <= settings.nearPlane ||
            !std::isfinite(settings.bias) || settings.bias < 0.0f ||
            !std::isfinite(settings.slopeBias) || settings.slopeBias < 0.0f ||
            settings.pcfRadius < 0 || settings.pcfRadius > 4 || !finite(cameraPosition))
            throw std::invalid_argument("Invalid Shadow Settings");

        ShadowFrame frame{};
        frame.settings = settings;
        int shadowCasterCount = 0;
        for (const ShadowLightInput& light : lights) {
            if (light.castsShadows)
                ++shadowCasterCount;
        }
        if (shadowCasterCount > 4)
            throw std::invalid_argument("At most four lights can cast shadows in Phase 11.5");

        const std::array<Vec3, 6> faceDirections{
            Vec3{ 1.0f, 0.0f, 0.0f }, Vec3{ -1.0f, 0.0f, 0.0f },
            Vec3{ 0.0f, 1.0f, 0.0f }, Vec3{ 0.0f, -1.0f, 0.0f },
            Vec3{ 0.0f, 0.0f, 1.0f }, Vec3{ 0.0f, 0.0f, -1.0f }
        };
        const std::array<Vec3, 6> faceUp{
            Vec3{ 0.0f, -1.0f, 0.0f }, Vec3{ 0.0f, -1.0f, 0.0f },
            Vec3{ 0.0f, 0.0f, 1.0f }, Vec3{ 0.0f, 0.0f, -1.0f },
            Vec3{ 0.0f, -1.0f, 0.0f }, Vec3{ 0.0f, -1.0f, 0.0f }
        };

        for (const ShadowLightInput& light : lights) {
            if (!light.castsShadows)
                continue;

            if (!finite(light.position) || !finite(light.emissionDirection) ||
                light.emissionDirection.lengthSquared() < 1.0e-12f ||
                !std::isfinite(light.range) || light.range <= settings.nearPlane ||
                !std::isfinite(light.outerConeDegrees) || light.outerConeDegrees <= 0.0f ||
                light.outerConeDegrees >= 89.0f)
                throw std::invalid_argument("Invalid shadow-casting light");

            ShadowLightMap shadow{};
            shadow.objectId = light.objectId;
            shadow.type = light.type;
            shadow.position = light.position;
            shadow.emissionDirection = light.emissionDirection.normalized();
            shadow.range = light.range;

            if (light.type == ShadowLightKind::Directional) {
                const Vec3 eye = cameraPosition - shadow.emissionDirection * settings.directionalDistance;
                const Mat4 view = Mat4::lookAt(eye, cameraPosition, safeUp(shadow.emissionDirection));
                const Mat4 projection = ortho(settings.directionalHalfExtent, settings.nearPlane,
                                              settings.directionalDistance * 2.0f);
                shadow.faces.push_back(makeFace(projection * view, settings.resolution, draws));
            } else if (light.type == ShadowLightKind::Spot) {
                const Mat4 view = Mat4::lookAt(light.position,
                                               light.position + shadow.emissionDirection,
                                               safeUp(shadow.emissionDirection));
                const Mat4 projection = Mat4::perspective(light.outerConeDegrees * 2.0f * Pi / 180.0f,
                                                           1.0f, settings.nearPlane, light.range);
                shadow.faces.push_back(makeFace(projection * view, settings.resolution, draws));
            } else if (light.type == ShadowLightKind::Point) {
                const Mat4 projection = Mat4::perspective(Pi * 0.5f, 1.0f,
                                                           settings.nearPlane, light.range);
                for (std::size_t face = 0; face < 6; ++face) {
                    const Mat4 view = Mat4::lookAt(light.position,
                        light.position + faceDirections[face], faceUp[face]);
                    shadow.faces.push_back(makeFace(projection * view, settings.resolution, draws));
                }
            } else {
                throw std::invalid_argument("Unknown Shadow Light type");
            }
            frame.lights.push_back(std::move(shadow));
        }
            return frame;
        }
    }


    ShadowFrame ShadowMapping::prepare( const std::vector<ShadowLightInput>& lights,
                                        const Vec3& cameraPosition, const ShadowSettings& settings ) {
        return makeShadowFrame(lights, nullptr, cameraPosition, settings);
    }


    ShadowFrame ShadowMapping::build( const std::vector<ShadowLightInput>& lights,
                                      const std::vector<ShadowDrawInput>& draws,
                                      const Vec3& cameraPosition, const ShadowSettings& settings ) {
        return makeShadowFrame(lights, &draws, cameraPosition, settings);
    }
}
