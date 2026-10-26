#pragma once

#include "math/Mat4.h"
#include "math/Vec2.h"
#include "math/Vec3.h"
#include "math/Vec4.h"

#include "assets/mesh/Vertex.h"

#include "render/backends/software/ClipSpaceClipper.h"
#include "render/backends/software/RasterVertex.h"
#include "render/backends/software/Viewport.h"

#include "render/lighting/Lighting.h"
#include "render/lighting/NormalMatrix3.h"


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
                    vertex.color,
                    vertex.normal
                };
            }


            static ClipVertex toClipSpace( const Vertex& vertex, const Mat4& model,
                                           const Mat4& modelViewProjection,
                                           const NormalMatrix3& normalMatrix ) {
                ClipVertex result = toClipSpace(vertex, modelViewProjection);

                const Vec4 worldPosition = model * Vec4{
                    vertex.position.x,
                    vertex.position.y,
                    vertex.position.z,
                    1.0f
                };

                result.worldPosition = Vec3{
                    worldPosition.x,
                    worldPosition.y,
                    worldPosition.z
                };

                result.normal = transformNormal(normalMatrix, vertex.normal);

                return result;
            }


            static RasterVertex toRasterSpace( const ClipVertex& vertex, const Viewport& viewport ) {
                const float inverseW = 1.0f / vertex.position.w;

                const Vec3 ndcPosition{
                    vertex.position.x * inverseW,
                    vertex.position.y * inverseW,
                    vertex.position.z * inverseW
                };

                RasterVertex result = viewport.transform(ndcPosition, vertex.color);


                // Texture Attributes

                result.uv = vertex.uv;
                result.inverseW = inverseW;

                result.uvOverW = Vec2{
                    vertex.uv.x * inverseW,
                    vertex.uv.y * inverseW
                };


                // World-space Normal

                result.normal = vertex.normal;
                result.normalOverW = vertex.normal * inverseW;


                // World-space Position

                result.worldPosition = vertex.worldPosition;
                result.worldPositionOverW = vertex.worldPosition * inverseW;

                return result;
            }


            static RasterVertex process( const Vertex& vertex, const Mat4& model,
                                         const Mat4& view, const Mat4& projection,
                                         const Viewport& viewport ) {
                const NormalMatrix3 normalMatrix = calculateNormalMatrix(model);

                const ClipVertex clipVertex = toClipSpace(
                    vertex, model, projection * view * model, normalMatrix
                );


                // Legacy API: No Triangle Clipping

                if (clipVertex.position.w == 0.0f) {
                    RasterVertex result{};

                    result.depth = 1.0f;
                    result.color = vertex.color;
                    result.uv = vertex.uv;

                    result.normal = clipVertex.normal;
                    result.normalOverW = Vec3{};

                    result.worldPosition = clipVertex.worldPosition;
                    result.worldPositionOverW = Vec3{};

                    result.inverseW = 0.0f;

                    return result;
                }

                return toRasterSpace(clipVertex, viewport);
            }
    };
}