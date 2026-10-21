#pragma once

#include <cmath>


namespace ve
{

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;


    constexpr Vec3() = default;


    constexpr Vec3(
        float xValue,
        float yValue,
        float zValue
    )
        : x(xValue),
          y(yValue),
          z(zValue)
    {
    }


    constexpr Vec3 operator+(const Vec3& other) const
    {
        return Vec3{
            x + other.x,
            y + other.y,
            z + other.z
        };
    }


    constexpr Vec3 operator-(const Vec3& other) const
    {
        return Vec3{
            x - other.x,
            y - other.y,
            z - other.z
        };
    }


    constexpr Vec3 operator*(float scalar) const
    {
        return Vec3{
            x * scalar,
            y * scalar,
            z * scalar
        };
    }


    constexpr Vec3 operator/(float scalar) const
    {
        return Vec3{
            x / scalar,
            y / scalar,
            z / scalar
        };
    }


    constexpr Vec3& operator+=(const Vec3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;

        return *this;
    }


    constexpr Vec3& operator-=(const Vec3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;

        return *this;
    }


    constexpr Vec3& operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;

        return *this;
    }


    constexpr Vec3& operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;

        return *this;
    }


    constexpr Vec3 operator-() const
    {
        return Vec3{
            -x,
            -y,
            -z
        };
    }


    constexpr float lengthSquared() const
    {
        return
            x * x +
            y * y +
            z * z;
    }


    float length() const
    {
        return std::sqrt(lengthSquared());
    }


    Vec3 normalized() const
    {
        const float len = length();

        if (len == 0.0f)
        {
            return Vec3{};
        }

        return *this / len;
    }
};


constexpr Vec3 operator*(
    float scalar,
    const Vec3& vector
)
{
    return vector * scalar;
}


constexpr float dot(
    const Vec3& a,
    const Vec3& b
)
{
    return
        a.x * b.x +
        a.y * b.y +
        a.z * b.z;
}


constexpr Vec3 cross(
    const Vec3& a,
    const Vec3& b
)
{
    return Vec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

}