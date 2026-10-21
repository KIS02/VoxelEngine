#pragma once


namespace ve
{

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;


    constexpr Vec2() = default;


    constexpr Vec2(
        float xValue,
        float yValue
    )
        : x(xValue),
          y(yValue)
    {
    }


    constexpr Vec2 operator+(
        const Vec2& other
    ) const
    {
        return Vec2{
            x + other.x,
            y + other.y
        };
    }


    constexpr Vec2 operator-(
        const Vec2& other
    ) const
    {
        return Vec2{
            x - other.x,
            y - other.y
        };
    }


    constexpr Vec2 operator*(
        float scalar
    ) const
    {
        return Vec2{
            x * scalar,
            y * scalar
        };
    }
};

}