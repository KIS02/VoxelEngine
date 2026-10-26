#include "render/lighting/NormalMatrix3.h"

#include <cmath>
#include <stdexcept>


namespace ve
{
    NormalMatrix3 calculateNormalMatrix( const Mat4& model ) {
        const double a = model(0, 0);
        const double b = model(0, 1);
        const double c = model(0, 2);
        const double d = model(1, 0);
        const double e = model(1, 1);
        const double f = model(1, 2);
        const double g = model(2, 0);
        const double h = model(2, 1);
        const double i = model(2, 2);

        const double c00 = e * i - f * h;
        const double c01 = f * g - d * i;
        const double c02 = d * h - e * g;
        const double c10 = c * h - b * i;
        const double c11 = a * i - c * g;
        const double c12 = b * g - a * h;
        const double c20 = b * f - c * e;
        const double c21 = c * d - a * f;
        const double c22 = a * e - b * d;

        const double determinant = a * c00 + b * c01 + c * c02;

        if (!std::isfinite(determinant) || std::fabs(determinant) < 1.0e-12)
            throw std::invalid_argument("Model matrix has a singular or near-singular normal transform");

        const double reciprocal = 1.0 / determinant;
        const NormalMatrix3 result{{
            { static_cast<float>(c00 * reciprocal), static_cast<float>(c01 * reciprocal), static_cast<float>(c02 * reciprocal) },
            { static_cast<float>(c10 * reciprocal), static_cast<float>(c11 * reciprocal), static_cast<float>(c12 * reciprocal) },
            { static_cast<float>(c20 * reciprocal), static_cast<float>(c21 * reciprocal), static_cast<float>(c22 * reciprocal) }
        }};

        for (const auto& row : result.m) {
            for (float element : row) {
                if (!std::isfinite(element))
                    throw std::invalid_argument("Normal matrix contains a non-finite value");
            }
        }

        return result;
    }
}