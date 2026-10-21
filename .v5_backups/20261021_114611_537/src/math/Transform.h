#pragma once

#include "math/Mat4.h"
#include "math/Vec3.h"


namespace ve
{

struct Transform
{
    Vec3 position{
        0.0f,
        0.0f,
        0.0f
    };


    Vec3 rotation{
        0.0f,
        0.0f,
        0.0f
    };


    Vec3 scale{
        1.0f,
        1.0f,
        1.0f
    };


    Mat4 matrix() const
    {
        const Mat4 translation =
            Mat4::translation(position);


        const Mat4 rotationX =
            Mat4::rotationX(rotation.x);

        const Mat4 rotationY =
            Mat4::rotationY(rotation.y);

        const Mat4 rotationZ =
            Mat4::rotationZ(rotation.z);


        const Mat4 scaling =
            Mat4::scale(scale);


        return
            translation
            *
            rotationZ
            *
            rotationY
            *
            rotationX
            *
            scaling;
    }
};

}