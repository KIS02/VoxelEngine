#pragma once


namespace ve
{
    struct LinearRGB
    {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;


        constexpr LinearRGB() = default;

        constexpr LinearRGB( float red, float green, float blue ) : r(red), g(green), b(blue) { }


        constexpr LinearRGB operator+( const LinearRGB& other ) const {
            return LinearRGB{ r + other.r, g + other.g, b + other.b };
        }

        constexpr LinearRGB operator*( const LinearRGB& other ) const {
            return LinearRGB{ r * other.r, g * other.g, b * other.b };
        }

        constexpr LinearRGB operator*( float scalar ) const {
            return LinearRGB{ r * scalar, g * scalar, b * scalar };
        }
    };
}