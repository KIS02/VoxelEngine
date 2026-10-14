#pragma once

#include "math/Mat4.h"
#include "math/Vec2.h"
#include "math/Vec3.h"
#include "math/Vec4.h"

#include "render/ClipSpaceClipper.h"
#include "render/RasterVertex.h"
#include "render/Vertex.h"
#include "render/Viewport.h"



namespace ve
{
    class VertexProcessor {
        public:
            static ClipVertex toClipSpace( const Vertex& vertex, const Mat4& modelViewProjection ) {
                const Vec4 localPosition{
                    vertex.position.x,
                    vertex.position.y,
                    vertex.position.z,
                    1.0f
                };



                return ClipVertex{
                    modelViewProjection * localPosition,
                    vertex.uv,
                    vertex.color
                };
            }



            static RasterVertex toRasterSpace( const ClipVertex& vertex, const Viewport& viewport ) {
                // This Function Must Be Called Only After Clipping

                const float inverseW = 1.0f / vertex.position.w;



                const Vec3 ndcPosition{
                    vertex.position.x * inverseW,
                    vertex.position.y * inverseW,
                    vertex.position.z * inverseW
                };



                RasterVertex result = viewport.transform(
                    ndcPosition,
                    vertex.color
                );



                // Preserve Attributes for Perspective-Correct Interpolation

                result.uv = vertex.uv;

                result.inverseW = inverseW;

                result.uvOverW = Vec2{
                    vertex.uv.x * inverseW,
                    vertex.uv.y * inverseW
                };



                return result;
            }



            static RasterVertex process( const Vertex& vertex, const Mat4& model,
                                         const Mat4& view, const Mat4& projection,
                                         const Viewport& viewport ) {
                // Legacy Single-Vertex Processing API

                const ClipVertex clipVertex = toClipSpace(
                    vertex,
                    projection * view * model
                );



                if (clipVertex.position.w == 0.0f) {
                    RasterVertex result{};

                    result.depth = 1.0f;
                    result.color = vertex.color;
                    result.uv = vertex.uv;
                    result.inverseW = 0.0f;

                    return result;
                }



                // The Single-Vertex API Does Not Clip Whole Triangles

                return toRasterSpace(clipVertex, viewport);
            }
    };
}