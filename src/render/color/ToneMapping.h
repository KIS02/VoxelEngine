#pragma once

#include <cstdint>

#include "math/Color.h"
#include "math/LinearRGB.h"


namespace ve
{
    enum class ToneMappingOperator : std::uint8_t {
        Reinhard, LinearClamp
    };


    struct ToneMappingSettings
    {
        bool enabled = false;
        float exposure = 1.0f;
        ToneMappingOperator method = ToneMappingOperator::Reinhard;
    };


    void validateToneMapping( const ToneMappingSettings& settings );
    LinearRGB toneMapLinear( const LinearRGB& hdr, const ToneMappingSettings& settings );
    Color32 toneMapToSrgb( const LinearRGB& hdr, std::uint8_t alpha,
                            const ToneMappingSettings& settings );
}
