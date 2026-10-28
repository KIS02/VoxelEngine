#include "render/backends/software/Rasterizer.h"
#include "render/backends/software/resources/HdrColorBuffer.h"

#include "render/backends/software/DepthBuffer.h"
#include "render/backends/software/FrameBuffer.h"
#include "assets/texture/Texture2D.h"
#include "render/backends/software/RasterInterpolation.h"
#include "render/backends/software/pipeline/shading/SoftwareFragmentShading.h"
#include "render/color/ColorSpace.h"
#include "render/lighting/Lighting.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>


namespace ve
{

    Rasterizer::Rasterizer(
        FrameBuffer& frameBuffer,
        DepthBuffer& depthBuffer
    )
        :
        frameBuffer_(frameBuffer),
        depthBuffer_(depthBuffer)
    {
    }


    void Rasterizer::drawLine(
        int x0,
        int y0,
        int x1,
        int y1,
        Color32 color
    )
    {
        const int dx =
            std::abs(
                x1 - x0
            );


        const int dy =
            std::abs(
                y1 - y0
            );


        const int stepX =
            (x0 < x1)
            ? 1
            : -1;


        const int stepY =
            (y0 < y1)
            ? 1
            : -1;


        int error =
            dx - dy;


        while (true)
        {
            frameBuffer_.setPixel(
                x0,
                y0,
                color
            );


            if (
                x0 == x1 &&
                y0 == y1
            )
            {
                break;
            }


            const int doubledError =
                error
                *
                2;


            if (doubledError > -dy)
            {
                error -= dy;

                x0 += stepX;
            }


            if (doubledError < dx)
            {
                error += dx;

                y0 += stepY;
            }
        }
    }


    void Rasterizer::drawTriangleWireframe(
        const Vec2& a,
        const Vec2& b,
        const Vec2& c,
        Color32 color
    )
    {
        drawLine(
            static_cast<int>(a.x),
            static_cast<int>(a.y),
            static_cast<int>(b.x),
            static_cast<int>(b.y),
            color
        );


        drawLine(
            static_cast<int>(b.x),
            static_cast<int>(b.y),
            static_cast<int>(c.x),
            static_cast<int>(c.y),
            color
        );


        drawLine(
            static_cast<int>(c.x),
            static_cast<int>(c.y),
            static_cast<int>(a.x),
            static_cast<int>(a.y),
            color
        );
    }


    float Rasterizer::edgeFunction(
        const Vec2& a,
        const Vec2& b,
        const Vec2& p
    )
    {
        return
            (p.x - a.x)
            *
            (b.y - a.y)
            -
            (p.y - a.y)
            *
            (b.x - a.x);
    }


    Vec3 Rasterizer::barycentric(
        const Vec2& a,
        const Vec2& b,
        const Vec2& c,
        const Vec2& p
    )
    {
        const float area =
            edgeFunction(
                a,
                b,
                c
            );


        if (area == 0.0f)
        {
            return Vec3{
                -1.0f,
                -1.0f,
                -1.0f
            };
        }


        const float alpha =
            edgeFunction(
                b,
                c,
                p
            )
            /
            area;


        const float beta =
            edgeFunction(
                c,
                a,
                p
            )
            /
            area;


        const float gamma =
            edgeFunction(
                a,
                b,
                p
            )
            /
            area;


        return Vec3{
            alpha,
            beta,
            gamma
        };
    }


    void Rasterizer::drawTriangleFilled(
        const Vec2& a,
        const Vec2& b,
        const Vec2& c,
        Color32 color
    )
    {
        const float area =
            edgeFunction(
                a,
                b,
                c
            );


        if (area == 0.0f)
        {
            return;
        }


        const float minXFloat =
            std::min({
                a.x,
                b.x,
                c.x
            });


        const float maxXFloat =
            std::max({
                a.x,
                b.x,
                c.x
            });


        const float minYFloat =
            std::min({
                a.y,
                b.y,
                c.y
            });


        const float maxYFloat =
            std::max({
                a.y,
                b.y,
                c.y
            });


        const int minX =
            static_cast<int>(
                std::floor(
                    minXFloat
                )
            );


        const int maxX =
            static_cast<int>(
                std::ceil(
                    maxXFloat
                )
            );


        const int minY =
            static_cast<int>(
                std::floor(
                    minYFloat
                )
            );


        const int maxY =
            static_cast<int>(
                std::ceil(
                    maxYFloat
                )
            );


        const int startX =
            std::max(
                minX,
                0
            );


        const int endX =
            std::min(
                maxX,
                frameBuffer_.width() - 1
            );


        const int startY =
            std::max(
                minY,
                0
            );


        const int endY =
            std::min(
                maxY,
                frameBuffer_.height() - 1
            );


        for (
            int y = startY;
            y <= endY;
            ++y
        )
        {
            for (
                int x = startX;
                x <= endX;
                ++x
            )
            {
                const Vec2 p{
                    static_cast<float>(x) + 0.5f,
                    static_cast<float>(y) + 0.5f
                };


                const Vec3 weights =
                    barycentric(
                        a,
                        b,
                        c,
                        p
                    );


                if (
                    weights.x < 0.0f ||
                    weights.y < 0.0f ||
                    weights.z < 0.0f
                )
                {
                    continue;
                }


                frameBuffer_.setPixel(
                    x,
                    y,
                    color
                );
            }
        }
    }


    void Rasterizer::drawTriangleInterpolated(
        const Vec2& a,
        Color32 colorA,

        const Vec2& b,
        Color32 colorB,

        const Vec2& c,
        Color32 colorC
    )
    {
        const float area =
            edgeFunction(
                a,
                b,
                c
            );


        if (area == 0.0f)
        {
            return;
        }


        const float minXFloat =
            std::min({
                a.x,
                b.x,
                c.x
            });


        const float maxXFloat =
            std::max({
                a.x,
                b.x,
                c.x
            });


        const float minYFloat =
            std::min({
                a.y,
                b.y,
                c.y
            });


        const float maxYFloat =
            std::max({
                a.y,
                b.y,
                c.y
            });


        const int minX =
            static_cast<int>(
                std::floor(
                    minXFloat
                )
            );


        const int maxX =
            static_cast<int>(
                std::ceil(
                    maxXFloat
                )
            );


        const int minY =
            static_cast<int>(
                std::floor(
                    minYFloat
                )
            );


        const int maxY =
            static_cast<int>(
                std::ceil(
                    maxYFloat
                )
            );


        const int startX =
            std::max(
                minX,
                0
            );


        const int endX =
            std::min(
                maxX,
                frameBuffer_.width() - 1
            );


        const int startY =
            std::max(
                minY,
                0
            );


        const int endY =
            std::min(
                maxY,
                frameBuffer_.height() - 1
            );


        for (
            int y = startY;
            y <= endY;
            ++y
        )
        {
            for (
                int x = startX;
                x <= endX;
                ++x
            )
            {
                const Vec2 p{
                    static_cast<float>(x) + 0.5f,
                    static_cast<float>(y) + 0.5f
                };


                const Vec3 weights =
                    barycentric(
                        a,
                        b,
                        c,
                        p
                    );


                if (
                    weights.x < 0.0f ||
                    weights.y < 0.0f ||
                    weights.z < 0.0f
                )
                {
                    continue;
                }


                const float red =
                    weights.x
                    *
                    static_cast<float>(
                        getRed(colorA)
                    )
                    +
                    weights.y
                    *
                    static_cast<float>(
                        getRed(colorB)
                    )
                    +
                    weights.z
                    *
                    static_cast<float>(
                        getRed(colorC)
                    );


                const float green =
                    weights.x
                    *
                    static_cast<float>(
                        getGreen(colorA)
                    )
                    +
                    weights.y
                    *
                    static_cast<float>(
                        getGreen(colorB)
                    )
                    +
                    weights.z
                    *
                    static_cast<float>(
                        getGreen(colorC)
                    );


                const float blue =
                    weights.x
                    *
                    static_cast<float>(
                        getBlue(colorA)
                    )
                    +
                    weights.y
                    *
                    static_cast<float>(
                        getBlue(colorB)
                    )
                    +
                    weights.z
                    *
                    static_cast<float>(
                        getBlue(colorC)
                    );


                const float alphaChannel =
                    weights.x
                    *
                    static_cast<float>(
                        getAlpha(colorA)
                    )
                    +
                    weights.y
                    *
                    static_cast<float>(
                        getAlpha(colorB)
                    )
                    +
                    weights.z
                    *
                    static_cast<float>(
                        getAlpha(colorC)
                    );


                const Color32 pixelColor =
                    makeColor(
                        static_cast<std::uint8_t>(
                            red
                        ),

                        static_cast<std::uint8_t>(
                            green
                        ),

                        static_cast<std::uint8_t>(
                            blue
                        ),

                        static_cast<std::uint8_t>(
                            alphaChannel
                        )
                    );


                frameBuffer_.setPixel(
                    x,
                    y,
                    pixelColor
                );
            }
        }
    }


    void Rasterizer::drawTriangle(
        const RasterVertex& a,
        const RasterVertex& b,
        const RasterVertex& c
    )
    {
        const float area =
            edgeFunction(
                a.position,
                b.position,
                c.position
            );


        if (area <= 0.0f)
        {
            return;
        }


        const float minXFloat =
            std::min({
                a.position.x,
                b.position.x,
                c.position.x
            });


        const float maxXFloat =
            std::max({
                a.position.x,
                b.position.x,
                c.position.x
            });


        const float minYFloat =
            std::min({
                a.position.y,
                b.position.y,
                c.position.y
            });


        const float maxYFloat =
            std::max({
                a.position.y,
                b.position.y,
                c.position.y
            });


        const int minX =
            static_cast<int>(
                std::floor(
                    minXFloat
                )
            );


        const int maxX =
            static_cast<int>(
                std::ceil(
                    maxXFloat
                )
            );


        const int minY =
            static_cast<int>(
                std::floor(
                    minYFloat
                )
            );


        const int maxY =
            static_cast<int>(
                std::ceil(
                    maxYFloat
                )
            );


        const int startX =
            std::max(
                minX,
                0
            );


        const int endX =
            std::min(
                maxX,
                frameBuffer_.width() - 1
            );


        const int startY =
            std::max(
                minY,
                0
            );


        const int endY =
            std::min(
                maxY,
                frameBuffer_.height() - 1
            );


        for (
            int y = startY;
            y <= endY;
            ++y
        )
        {
            for (
                int x = startX;
                x <= endX;
                ++x
            )
            {
                const Vec2 p{
                    static_cast<float>(x) + 0.5f,
                    static_cast<float>(y) + 0.5f
                };


                const Vec3 weights =
                    barycentric(
                        a.position,
                        b.position,
                        c.position,
                        p
                    );


                if (
                    weights.x < 0.0f ||
                    weights.y < 0.0f ||
                    weights.z < 0.0f
                )
                {
                    continue;
                }


                const float depth =
                    weights.x
                    *
                    a.depth
                    +
                    weights.y
                    *
                    b.depth
                    +
                    weights.z
                    *
                    c.depth;


                const float currentDepth =
                    depthBuffer_.depth(
                        x,
                        y
                    );


                if (depth >= currentDepth)
                {
                    continue;
                }


                const float red =
                    weights.x
                    *
                    static_cast<float>(
                        getRed(a.color)
                    )
                    +
                    weights.y
                    *
                    static_cast<float>(
                        getRed(b.color)
                    )
                    +
                    weights.z
                    *
                    static_cast<float>(
                        getRed(c.color)
                    );


                const float green =
                    weights.x
                    *
                    static_cast<float>(
                        getGreen(a.color)
                    )
                    +
                    weights.y
                    *
                    static_cast<float>(
                        getGreen(b.color)
                    )
                    +
                    weights.z
                    *
                    static_cast<float>(
                        getGreen(c.color)
                    );


                const float blue =
                    weights.x
                    *
                    static_cast<float>(
                        getBlue(a.color)
                    )
                    +
                    weights.y
                    *
                    static_cast<float>(
                        getBlue(b.color)
                    )
                    +
                    weights.z
                    *
                    static_cast<float>(
                        getBlue(c.color)
                    );


                const float alphaChannel =
                    weights.x
                    *
                    static_cast<float>(
                        getAlpha(a.color)
                    )
                    +
                    weights.y
                    *
                    static_cast<float>(
                        getAlpha(b.color)
                    )
                    +
                    weights.z
                    *
                    static_cast<float>(
                        getAlpha(c.color)
                    );


                const Color32 pixelColor =
                    makeColor(
                        static_cast<std::uint8_t>(
                            red
                        ),

                        static_cast<std::uint8_t>(
                            green
                        ),

                        static_cast<std::uint8_t>(
                            blue
                        ),

                        static_cast<std::uint8_t>(
                            alphaChannel
                        )
                    );


                depthBuffer_.setDepth(
                    x,
                    y,
                    depth
                );


                frameBuffer_.setPixel(
                    x,
                    y,
                    pixelColor
                );
            }
        }
    }

    void Rasterizer::drawTriangleTextured( const RasterVertex& a, const RasterVertex& b,
                                        const RasterVertex& c, const Texture2D& texture,
                                        const DirectionalLight* light, const MaterialData* material,
                                        const Vec3* cameraPosition,
                                        const LightingSettings* sceneLighting,
                                           const ShadowVisibility* shadow, HdrColorBuffer* hdr,
                                           bool lightingValidated ) {
        const float area = edgeFunction(a.position, b.position, c.position);

        if (area <= 0.0f)
            return;


        // Calculate Triangle Bounding Box

        const float minXFloat = std::min({ a.position.x, b.position.x, c.position.x });
        const float maxXFloat = std::max({ a.position.x, b.position.x, c.position.x });

        const float minYFloat = std::min({ a.position.y, b.position.y, c.position.y });
        const float maxYFloat = std::max({ a.position.y, b.position.y, c.position.y });

        const int minX = static_cast<int>(std::floor(minXFloat));
        const int maxX = static_cast<int>(std::ceil(maxXFloat));

        const int minY = static_cast<int>(std::floor(minYFloat));
        const int maxY = static_cast<int>(std::ceil(maxYFloat));


        // Clamp Bounding Box to FrameBuffer

        const int startX = std::max(minX, 0);
        const int endX = std::min(maxX, frameBuffer_.width() - 1);

        const int startY = std::max(minY, 0);
        const int endY = std::min(maxY, frameBuffer_.height() - 1);


        // Rasterize Every Covered Pixel

        for (int y = startY; y <= endY; ++y) {
            for (int x = startX; x <= endX; ++x) {
                const Vec2 p{
                    static_cast<float>(x) + 0.5f,
                    static_cast<float>(y) + 0.5f
                };


                // Calculate Barycentric Coordinates

                const Vec3 weights = barycentric(
                    a.position, b.position, c.position, p
                );

                if (weights.x < 0.0f || weights.y < 0.0f || weights.z < 0.0f)
                    continue;


                // Interpolate Depth

                const float depth =
                    weights.x * a.depth +
                    weights.y * b.depth +
                    weights.z * c.depth;

                const float currentDepth = depthBuffer_.depth(x, y);

                if (depth >= currentDepth)
                    continue;


                // Interpolate Inverse W

                const float interpolatedInverseW =
                    weights.x * a.inverseW +
                    weights.y * b.inverseW +
                    weights.z * c.inverseW;

                if (interpolatedInverseW == 0.0f)
                    continue;


                // Perspective-Correct Texture Coordinates

                const float uOverW =
                    weights.x * a.uvOverW.x +
                    weights.y * b.uvOverW.x +
                    weights.z * c.uvOverW.x;

                const float vOverW =
                    weights.x * a.uvOverW.y +
                    weights.y * b.uvOverW.y +
                    weights.z * c.uvOverW.y;

                const Vec2 uv{
                    uOverW / interpolatedInverseW,
                    vOverW / interpolatedInverseW
                };


                // Sample Texture

                const Color32 textureColor = texture.sampleNearest(uv);
                Color32 pixelColor = textureColor;
                LinearRGB hdrColor{};


                // Execute the Software Fragment Shading stage.

                if (light != nullptr) {
                    const Vec3 normal = interpolatePerspectiveNormal(
                        a, b, c, weights, interpolatedInverseW
                    );

                    Vec3 worldPosition{};

                    if (material != nullptr && cameraPosition != nullptr)
                        worldPosition = interpolatePerspectiveWorldPosition(
                            a, b, c, weights, interpolatedInverseW
                        );

                    const bool useFastPath = lightingValidated && sceneLighting != nullptr &&
                                             material != nullptr && cameraPosition != nullptr;
                    if (useFastPath) {
                        const LinearRGB linearColor = shadeLinearMultiLightValidated(
                            textureColor, normal, worldPosition, *cameraPosition,
                            *sceneLighting, *material, shadow
                        );
                        if (hdr != nullptr)
                            hdrColor = linearColor;
                        else
                            pixelColor = encodeSrgb(linearColor, getAlpha(textureColor));
                    } else if (hdr != nullptr) {
                        hdrColor = SoftwareFragmentShading::shadeLinear(
                            textureColor, normal, worldPosition, cameraPosition,
                            light, material, sceneLighting, shadow
                        );
                    } else {
                        pixelColor = SoftwareFragmentShading::shade(
                            textureColor, normal, worldPosition, cameraPosition,
                            light, material, sceneLighting, shadow
                        );
                    }
                }



                // Update Depth and Color

                depthBuffer_.setDepth(x, y, depth);
                if (hdr != nullptr)
                    hdr->setPixel(x, y, HdrPixel{ hdrColor, getAlpha(textureColor) });
                else
                    frameBuffer_.setPixel(x, y, pixelColor);
            }
        }
    }

}
