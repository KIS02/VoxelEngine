#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "math/Vec2.h"
#include "math/Vec4.h"

#include "math/Color.h"



namespace ve
{
    struct ClipVertex
    {
        Vec4 position{};
        Vec2 uv{};

        Color32 color = makeColor(255, 255, 255);
    };



    class ClipSpaceClipper {
        private:
            static double planeDistance( const ClipVertex& vertex, int plane ) {
                const Vec4& p = vertex.position;

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



            static ClipVertex interpolate( const ClipVertex& a, const ClipVertex& b, double fraction ) {
                const float t = static_cast<float>(std::clamp(fraction, 0.0, 1.0));

                const auto lerp = [t]( float start, float end ) {
                    return start + (end - start) * t;
                };

                const auto lerpColor = [&]( std::uint8_t start, std::uint8_t end ) {
                    return static_cast<std::uint8_t>(
                        std::clamp(
                            lerp(static_cast<float>(start), static_cast<float>(end)),
                            0.0f,
                            255.0f
                        )
                    );
                };

                return ClipVertex{
                    Vec4{
                        lerp(a.position.x, b.position.x),
                        lerp(a.position.y, b.position.y),
                        lerp(a.position.z, b.position.z),
                        lerp(a.position.w, b.position.w)
                    },
                    Vec2{
                        lerp(a.uv.x, b.uv.x),
                        lerp(a.uv.y, b.uv.y)
                    },
                    makeColor(
                        lerpColor(getRed(a.color), getRed(b.color)),
                        lerpColor(getGreen(a.color), getGreen(b.color)),
                        lerpColor(getBlue(a.color), getBlue(b.color)),
                        lerpColor(getAlpha(a.color), getAlpha(b.color))
                    )
                };
            }


        public:
            static std::size_t clipTriangle( const ClipVertex& a, const ClipVertex& b, const ClipVertex& c,
                                             std::array<ClipVertex, 16>& output ) {
                std::array<ClipVertex, 16> current{};
                std::array<ClipVertex, 16> next{};

                current[0] = a;
                current[1] = b;
                current[2] = c;

                std::size_t count = 3;



                for (int plane = 0; plane < 6; ++plane) {
                    std::size_t nextCount = 0;

                    ClipVertex previous = current[count - 1];

                    double previousDistance = planeDistance(previous, plane);
                    bool previousInside = previousDistance >= 0.0;



                    for (std::size_t i = 0; i < count; ++i) {
                        const ClipVertex& vertex = current[i];

                        const double distance = planeDistance(vertex, plane);
                        const bool inside = distance >= 0.0;



                        if (inside != previousInside) {
                            const double fraction = previousDistance / (previousDistance - distance);

                            next[nextCount++] = interpolate(previous, vertex, fraction);
                        }



                        if (inside) {
                            next[nextCount++] = vertex;
                        }



                        previous = vertex;
                        previousDistance = distance;
                        previousInside = inside;
                    }



                    if (nextCount < 3) {
                        return 0;
                    }



                    current = next;
                    count = nextCount;
                }



                output = current;

                return count;
            }
    };
}