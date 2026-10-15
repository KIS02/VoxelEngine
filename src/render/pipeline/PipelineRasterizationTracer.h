
#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

#include "math/Vec2.h"
#include "math/Vec3.h"

#include "render/pipeline/PipelineTrace.h"


namespace ve
{
    class PipelineRasterizationTracer {
        private:
            static bool isFinite( const Vec2& value ) {
                return std::isfinite(value.x) && std::isfinite(value.y);
            }

            static bool isFinite( const Vec3& value ) {
                return std::isfinite(value.x) && std::isfinite(value.y) &&
                       std::isfinite(value.z);
            }


            // Same Edge Function Convention as Rasterizer

            static float edgeFunction( const Vec2& a, const Vec2& b, const Vec2& p ) {
                return (p.x - a.x) * (b.y - a.y) -
                       (p.y - a.y) * (b.x - a.x);
            }


            // Same Barycentric Calculation as Rasterizer

            static Vec3 barycentric( const Vec2& a, const Vec2& b, const Vec2& c,
                                     const Vec2& p, float area ) {
                const float alpha = edgeFunction(b, c, p) / area;
                const float beta = edgeFunction(c, a, p) / area;
                const float gamma = edgeFunction(a, b, p) / area;

                return Vec3{ alpha, beta, gamma };
            }


        public:
            static bool trace( PipelineTrace& result, int x, int y ) {
                if (!result.triangle.has_value() || !result.screenPolygon.has_value())
                    return false;

                if (!result.isConsistent())
                    return false;


                // Validate the Requested Framebuffer Pixel

                const int width = result.triangle->framebufferWidth;
                const int height = result.triangle->framebufferHeight;

                if (x < 0 || y < 0 || x >= width || y >= height)
                    return false;

                const std::vector<PipelineScreenVertex>& polygon = *result.screenPolygon;

                if (polygon.size() < 3)
                    return false;


                // Validate Every Screen-space Vertex Before Sampling

                for (const PipelineScreenVertex& vertex : polygon) {
                    if (!isFinite(vertex.position) || !isFinite(vertex.uv) ||
                        !std::isfinite(vertex.depth) || !std::isfinite(vertex.inverseW))
                        return false;

                    if (vertex.inverseW <= 0.0f)
                        return false;
                }


                // Rasterizer Tests the Center of Each Pixel

                const Vec2 point{
                    static_cast<float>(x) + 0.5f,
                    static_cast<float>(y) + 0.5f
                };

                if (!isFinite(point))
                    return false;


                // Prepare a Fresh Pixel Sample

                PipelinePixelSample sample{};

                sample.x = x;
                sample.y = y;


                // Triangulate the Clipped Polygon as a Triangle Fan

                for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
                    const PipelineScreenVertex& a = polygon[0];
                    const PipelineScreenVertex& b = polygon[i];
                    const PipelineScreenVertex& c = polygon[i + 1];


                    // Match Rasterizer's Backface and Degenerate Rejection

                    const float area = edgeFunction(a.position, b.position, c.position);

                    if (!std::isfinite(area))
                        return false;

                    if (area <= 0.0f)
                        continue;


                    // Calculate Barycentric Weights

                    const Vec3 weights = barycentric(
                        a.position, b.position, c.position, point, area
                    );

                    if (!isFinite(weights))
                        return false;

                    if (weights.x < 0.0f || weights.y < 0.0f || weights.z < 0.0f)
                        continue;


                    // Interpolate Depth in Screen Space

                    const float depth = weights.x * a.depth +
                                        weights.y * b.depth +
                                        weights.z * c.depth;

                    if (!std::isfinite(depth))
                        return false;


                    // Calculate the Interpolated Reciprocal W

                    const float interpolatedInverseW =
                        weights.x * a.inverseW +
                        weights.y * b.inverseW +
                        weights.z * c.inverseW;

                    if (!std::isfinite(interpolatedInverseW))
                        return false;

                    if (interpolatedInverseW == 0.0f)
                        continue;


                    // Perspective-correct UV Interpolation

                    const float uOverW =
                        weights.x * (a.uv.x * a.inverseW) +
                        weights.y * (b.uv.x * b.inverseW) +
                        weights.z * (c.uv.x * c.inverseW);

                    const float vOverW =
                        weights.x * (a.uv.y * a.inverseW) +
                        weights.y * (b.uv.y * b.inverseW) +
                        weights.z * (c.uv.y * c.inverseW);

                    const Vec2 uv{
                        uOverW / interpolatedInverseW,
                        vOverW / interpolatedInverseW
                    };

                    if (!isFinite(uv))
                        return false;


                    // Record the First Rasterized Triangle Covering This Pixel

                    sample.covered = true;
                    sample.barycentric = weights;
                    sample.uv = uv;
                    sample.incomingDepth = depth;

                    break;
                }


                // Depth Testing and Texture Sampling Belong to Phase 8.7

                result.pixelSample = sample;

                return true;
            }
    };
}
