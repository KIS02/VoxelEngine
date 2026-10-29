#pragma once

#include <cstdint>


namespace ve
{
    enum class ShadingModel : std::uint8_t {
        BlinnPhong,
        Pbr
    };
}
