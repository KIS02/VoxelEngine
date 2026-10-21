#pragma once

#include <cstdint>


namespace ve
{
    struct SceneObjectId
    {
        std::uint64_t value = 0;


        constexpr bool valid() const {
            return value != 0;
        }

        constexpr bool operator==( const SceneObjectId& other ) const {
            return value == other.value;
        }
    };
}
