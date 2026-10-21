#pragma once

#include <cstdint>


namespace ve
{
    struct EntityId
    {
        std::uint64_t value = 0;


        constexpr bool valid() const {
            return value != 0;
        }
    };
}
