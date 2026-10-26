#pragma once

#include "math/Mat4.h"


namespace ve
{
    struct NormalMatrix3
    {
        float m[3][3]{};
    };


    NormalMatrix3 calculateNormalMatrix( const Mat4& model );
}