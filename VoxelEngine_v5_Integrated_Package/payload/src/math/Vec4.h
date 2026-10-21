#pragma once

#include "math/Vec3.h"


namespace ve
{

struct Vec4
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;


    constexpr Vec4() = default;


    constexpr Vec4(
        float xValue,
        float yValue,
        float zValue,
        float wValue
    )
        : x(xValue),
          y(yValue),
          z(zValue),
          w(wValue)
    {
    }


    constexpr Vec4(
        const Vec3& xyz,
        float wValue
    )
        : x(xyz.x),
          y(xyz.y),
          z(xyz.z),
          w(wValue)
    {
    }


    constexpr Vec4 operator+(const Vec4& other) const
    {
        return Vec4{
            x + other.x,
            y + other.y,
            z + other.z,
            w + other.w
        };
    }


    constexpr Vec4 operator-(const Vec4& other) const
    {
        return Vec4{
            x - other.x,
            y - other.y,
            z - other.z,
            w - other.w
        };
    }


    constexpr Vec4 operator*(float scalar) const
    {
        return Vec4{
            x * scalar,
            y * scalar,
            z * scalar,
            w * scalar
        };
    }


    constexpr Vec4 operator/(float scalar) const
    {
        return Vec4{
            x / scalar,
            y / scalar,
            z / scalar,
            w / scalar
        };
    }


    constexpr Vec4& operator+=(const Vec4& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;

        return *this;
    }


    constexpr Vec4& operator-=(const Vec4& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;

        return *this;
    }


    constexpr Vec4& operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;

        return *this;
    }


    constexpr Vec4& operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;

        return *this;
    }


    constexpr Vec4 operator-() const
    {
        return Vec4{
            -x,
            -y,
            -z,
            -w
        };
    }


    constexpr Vec3 vec4to3() const
    {
        return Vec3{
            x,
            y,
            z
        };
    }
};


constexpr Vec4 operator*( float scalar, const Vec4& vector ) {
    return vector * scalar;
}

}