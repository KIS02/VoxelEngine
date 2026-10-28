#pragma once

#include <cstdint>

#include "math/Vec3.h"


namespace ve
{
    class ShadowVisibility {
        public:
            virtual ~ShadowVisibility() = default;

            virtual float visibility( std::uint64_t objectId, const Vec3& worldPosition,
                                      const Vec3& worldNormal, const Vec3& directionToLight ) const = 0;
    };
}
