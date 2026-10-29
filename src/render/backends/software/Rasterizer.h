#pragma once

#include "math/Vec2.h"
#include "math/Vec3.h"
#include "math/Color.h"
#include "render/backends/software/RasterVertex.h"
#include "render/frame/ShadingModel.h"


namespace ve
{
    class FrameBuffer;
    class DepthBuffer;
    class Texture2D;

    struct DirectionalLight;
    struct MaterialData;
    struct LightingSettings;
    class ShadowVisibility;
    class HdrColorBuffer;


    class Rasterizer {
        private:
            FrameBuffer& frameBuffer_;
            DepthBuffer& depthBuffer_;


        public:
            Rasterizer( FrameBuffer& frameBuffer, DepthBuffer& depthBuffer );

            void drawLine( int x0, int y0, int x1, int y1, Color32 color );

            void drawTriangleWireframe( const Vec2& a, const Vec2& b,
                                        const Vec2& c, Color32 color );

            void drawTriangleFilled( const Vec2& a, const Vec2& b,
                                     const Vec2& c, Color32 color );

            void drawTriangleInterpolated( const Vec2& a, Color32 colorA,
                                            const Vec2& b, Color32 colorB,
                                            const Vec2& c, Color32 colorC );

            void drawTriangle( const RasterVertex& a, const RasterVertex& b,
                               const RasterVertex& c );

            void drawTriangleTextured( const RasterVertex& a, const RasterVertex& b,
                                       const RasterVertex& c, const Texture2D& texture,
                                       const DirectionalLight* light = nullptr,
                                       const MaterialData* material = nullptr,
                                       const Vec3* cameraPosition = nullptr,
                                       const LightingSettings* sceneLighting = nullptr,
                                       const ShadowVisibility* shadow = nullptr,
                                       HdrColorBuffer* hdr = nullptr,
                                       bool lightingValidated = false,
                                       ShadingModel shadingModel = ShadingModel::BlinnPhong );


        private:
            static float edgeFunction( const Vec2& a, const Vec2& b, const Vec2& p );

            static Vec3 barycentric( const Vec2& a, const Vec2& b,
                                     const Vec2& c, const Vec2& p );
    };
}
