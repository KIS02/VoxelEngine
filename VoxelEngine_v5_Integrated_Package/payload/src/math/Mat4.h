#pragma once

#include <cmath>

#include "math/Vec3.h"
#include "math/Vec4.h"


namespace ve
{

struct Mat4
{
    float m[4][4]{};


    constexpr Mat4() = default;


    constexpr float& operator()(
        int row,
        int column
    )
    {
        return m[row][column];
    }


    constexpr const float& operator()(
        int row,
        int column
    ) const
    {
        return m[row][column];
    }


    static constexpr Mat4 identity()
    {
        Mat4 result{};

        result(0, 0) = 1.0f;
        result(1, 1) = 1.0f;
        result(2, 2) = 1.0f;
        result(3, 3) = 1.0f;

        return result;
    }


    static constexpr Mat4 translation(
        const Vec3& translation
    )
    {
        Mat4 result = identity();

        result(0, 3) = translation.x;
        result(1, 3) = translation.y;
        result(2, 3) = translation.z;

        return result;
    }


    static constexpr Mat4 scale(
        const Vec3& scale
    )
    {
        Mat4 result = identity();

        result(0, 0) = scale.x;
        result(1, 1) = scale.y;
        result(2, 2) = scale.z;

        return result;
    }


    constexpr Vec4 operator*(
        const Vec4& vector
    ) const
    {
        return Vec4{
            m[0][0] * vector.x +
            m[0][1] * vector.y +
            m[0][2] * vector.z +
            m[0][3] * vector.w,

            m[1][0] * vector.x +
            m[1][1] * vector.y +
            m[1][2] * vector.z +
            m[1][3] * vector.w,

            m[2][0] * vector.x +
            m[2][1] * vector.y +
            m[2][2] * vector.z +
            m[2][3] * vector.w,

            m[3][0] * vector.x +
            m[3][1] * vector.y +
            m[3][2] * vector.z +
            m[3][3] * vector.w
        };
    }


    constexpr Mat4 operator*(
        const Mat4& other
    ) const
    {
        Mat4 result{};

        for (int row = 0; row < 4; ++row)
        {
            for (int column = 0; column < 4; ++column)
            {
                for (int k = 0; k < 4; ++k)
                {
                    result(row, column) +=
                        m[row][k] *
                        other(k, column);
                }
            }
        }

        return result;
    }

    static Mat4 rotationX(float radians)
    {
        Mat4 result = identity();

        const float c = std::cos(radians);
        const float s = std::sin(radians);

        result(1, 1) = c;
        result(1, 2) = -s;

        result(2, 1) = s;
        result(2, 2) = c;

        return result;
    }

    static Mat4 rotationY(float radians)
    {
        Mat4 result = identity();

        const float c = std::cos(radians);
        const float s = std::sin(radians);

        result(0, 0) = c;
        result(0, 2) = s;

        result(2, 0) = -s;
        result(2, 2) = c;

        return result;
    }

    static Mat4 rotationZ(float radians)
    {
        Mat4 result = identity();

        const float c = std::cos(radians);
        const float s = std::sin(radians);

        result(0, 0) = c;
        result(0, 1) = -s;

        result(1, 0) = s;
        result(1, 1) = c;

        return result;
    }

    static Mat4 lookAt( const Vec3& eye, const Vec3& target, const Vec3& up ) {
        const Vec3 forward = (target - eye).normalized();

        const Vec3 right = cross(forward, up).normalized();

        const Vec3 cameraUp = cross(right, forward);


        Mat4 result = identity();


        result(0, 0) = right.x;
        result(0, 1) = right.y;
        result(0, 2) = right.z;

        result(0, 3) = -dot(right, eye);


        result(1, 0) = cameraUp.x;
        result(1, 1) = cameraUp.y;
        result(1, 2) = cameraUp.z;

        result(1, 3) = -dot(cameraUp, eye);


        result(2, 0) = -forward.x;
        result(2, 1) = -forward.y;
        result(2, 2) = -forward.z;

        result(2, 3) = dot(forward, eye);


        return result;
    }

    static Mat4 perspective( float fovYRadians, float aspectRatio, float nearPlane, float farPlane )
    {
        Mat4 result{};


        const float tanHalfFov =
            std::tan(
                fovYRadians * 0.5f
            );


        result(0, 0) =
            1.0f
            /
            (aspectRatio * tanHalfFov);


        result(1, 1) =
            1.0f
            /
            tanHalfFov;


        result(2, 2) =
            -(farPlane + nearPlane)
            /
            (farPlane - nearPlane);


        result(2, 3) =
            -(2.0f
            * farPlane
            * nearPlane)
            /
            (farPlane - nearPlane);


        result(3, 2) =
            -1.0f;


        return result;
    }
};



}