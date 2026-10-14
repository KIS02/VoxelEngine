#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>

#include "math/Mat4.h"
#include "math/Vec3.h"
#include "math/Vec4.h"



namespace ve
{
    struct ProjectedLine
    {
        Vec3 start{};
        Vec3 end{};
    };



    struct ProjectedSelectionOutline
    {
        std::array<ProjectedLine, 12> lines{};
        std::size_t count = 0;
    };



    class SelectionOutlineGeometry {
        private:
            static double planeDistance( const Vec4& p, int plane ) {
                switch (plane) {
                    case 0: return static_cast<double>(p.w) + p.x;
                    case 1: return static_cast<double>(p.w) - p.x;
                    case 2: return static_cast<double>(p.w) + p.y;
                    case 3: return static_cast<double>(p.w) - p.y;
                    case 4: return static_cast<double>(p.w) + p.z;
                    case 5: return static_cast<double>(p.w) - p.z;
                }

                return 0.0;
            }



            static bool finite( const Vec4& p ) {
                return std::isfinite(p.x) && std::isfinite(p.y) &&
                       std::isfinite(p.z) && std::isfinite(p.w);
            }



            static Vec4 interpolate( const Vec4& a, const Vec4& b, double t ) {
                const double complement = 1.0 - t;

                return Vec4{
                    static_cast<float>(complement * a.x + t * b.x),
                    static_cast<float>(complement * a.y + t * b.y),
                    static_cast<float>(complement * a.z + t * b.z),
                    static_cast<float>(complement * a.w + t * b.w)
                };
            }



            static bool clipLine( Vec4& a, Vec4& b ) {
                if (!finite(a) || !finite(b)) {
                    return false;
                }

                for (int plane = 0; plane < 6; ++plane) {
                    const double distanceA = planeDistance(a, plane);
                    const double distanceB = planeDistance(b, plane);

                    if (distanceA < 0.0 && distanceB < 0.0) {
                        return false;
                    }

                    if (distanceA >= 0.0 && distanceB >= 0.0) {
                        continue;
                    }

                    const double t = distanceA / (distanceA - distanceB);
                    const Vec4 intersection = interpolate(a, b, t);

                    if (distanceA < 0.0) a = intersection;
                    else b = intersection;
                }

                constexpr float minimumW = 0.000001f;
                return finite(a) && finite(b) && a.w > minimumW && b.w > minimumW;
            }



            static Vec3 perspectiveDivide( const Vec4& clip ) {
                const float inverseW = 1.0f / clip.w;
                return Vec3{ clip.x * inverseW, clip.y * inverseW, clip.z * inverseW };
            }


        public:
            static ProjectedSelectionOutline create( const Vec3& blockMin, float expansion,
                                                     const Mat4& view, const Mat4& projection ) {
                if (!std::isfinite(blockMin.x) || !std::isfinite(blockMin.y) ||
                    !std::isfinite(blockMin.z) || !std::isfinite(expansion) || expansion < 0.0f) {
                    throw std::invalid_argument("Invalid selection outline geometry");
                }

                const float x0 = blockMin.x - expansion;
                const float y0 = blockMin.y - expansion;
                const float z0 = blockMin.z - expansion;
                const float x1 = blockMin.x + 1.0f + expansion;
                const float y1 = blockMin.y + 1.0f + expansion;
                const float z1 = blockMin.z + 1.0f + expansion;

                const std::array<Vec3, 8> corners{
                    Vec3{ x0, y0, z0 }, Vec3{ x1, y0, z0 },
                    Vec3{ x0, y1, z0 }, Vec3{ x1, y1, z0 },
                    Vec3{ x0, y0, z1 }, Vec3{ x1, y0, z1 },
                    Vec3{ x0, y1, z1 }, Vec3{ x1, y1, z1 }
                };

                constexpr std::array<std::array<int, 2>, 12> edges{
                    std::array<int, 2>{ 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 },
                    { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },
                    { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
                };

                const Mat4 viewProjection = projection * view;
                std::array<Vec4, 8> clipCorners{};

                for (std::size_t i = 0; i < corners.size(); ++i) {
                    const Vec3& p = corners[i];
                    clipCorners[i] = viewProjection * Vec4{ p.x, p.y, p.z, 1.0f };
                }

                ProjectedSelectionOutline result{};

                for (const auto& edge : edges) {
                    Vec4 a = clipCorners[edge[0]];
                    Vec4 b = clipCorners[edge[1]];

                    if (!clipLine(a, b)) {
                        continue;
                    }

                    const Vec3 start = perspectiveDivide(a);
                    const Vec3 end = perspectiveDivide(b);

                    if (!std::isfinite(start.x) || !std::isfinite(start.y) || !std::isfinite(start.z) ||
                        !std::isfinite(end.x) || !std::isfinite(end.y) || !std::isfinite(end.z)) {
                        continue;
                    }

                    result.lines[result.count++] = ProjectedLine{ start, end };
                }

                return result;
            }
    };
}
