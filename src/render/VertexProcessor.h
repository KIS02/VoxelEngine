#pragma once

#include "math/Mat4.h"
#include "math/Vec2.h"
#include "math/Vec3.h"
#include "math/Vec4.h"

#include "render/RasterVertex.h"
#include "render/Vertex.h"
#include "render/Viewport.h"


namespace ve
{

class VertexProcessor
{
public:

    static RasterVertex process(
        const Vertex& vertex,
        const Mat4& model,
        const Mat4& view,
        const Mat4& projection,
        const Viewport& viewport
    )
    {
        const Vec4 localPosition{
            vertex.position.x,
            vertex.position.y,
            vertex.position.z,
            1.0f
        };


        const Vec4 worldPosition =
            model
            *
            localPosition;


        const Vec4 viewPosition =
            view
            *
            worldPosition;


        const Vec4 clipPosition =
            projection
            *
            viewPosition;


        if (clipPosition.w == 0.0f)
        {
            RasterVertex rasterVertex{};

            rasterVertex.position =
                Vec2{
                    0.0f,
                    0.0f
                };

            rasterVertex.depth =
                1.0f;

            rasterVertex.color =
                vertex.color;

            rasterVertex.uv =
                vertex.uv;

            rasterVertex.inverseW =
                0.0f;

            rasterVertex.uvOverW =
                Vec2{
                    0.0f,
                    0.0f
                };

            return rasterVertex;
        }


        const float inverseW =
            1.0f
            /
            clipPosition.w;


        const Vec3 ndcPosition{
            clipPosition.x
            *
            inverseW,

            clipPosition.y
            *
            inverseW,

            clipPosition.z
            *
            inverseW
        };


        RasterVertex rasterVertex =
            viewport.transform(
                ndcPosition,
                vertex.color
            );


        rasterVertex.uv =
            vertex.uv;


        rasterVertex.inverseW =
            inverseW;


        rasterVertex.uvOverW =
            Vec2{
                vertex.uv.x
                *
                inverseW,

                vertex.uv.y
                *
                inverseW
            };


        return rasterVertex;
    }
};

}