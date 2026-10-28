#include "render/color/ToneMapping.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "render/color/ColorSpace.h"


namespace ve
{
    void validateToneMapping( const ToneMappingSettings& settings ) {
        if (!std::isfinite(settings.exposure) || settings.exposure <= 0.0f)
            throw std::invalid_argument("Tone Mapping exposure must be finite and positive");

        if (settings.method != ToneMappingOperator::Reinhard &&
            settings.method != ToneMappingOperator::LinearClamp)
            throw std::invalid_argument("Unknown Tone Mapping operator");
    }


    LinearRGB toneMapLinear( const LinearRGB& hdr, const ToneMappingSettings& settings ) {
        validateToneMapping(settings);

        const auto process = [&settings]( float value ) {
            if (!std::isfinite(value) || value < 0.0f)
                throw std::invalid_argument("HDR color must be finite and nonnegative");

            const float exposed = value * settings.exposure;
            if (!std::isfinite(exposed))
                throw std::overflow_error("Exposed HDR color overflow");

            if (settings.method == ToneMappingOperator::Reinhard)
                return exposed / (1.0f + exposed);

            return std::clamp(exposed, 0.0f, 1.0f);
        };

        return LinearRGB{ process(hdr.r), process(hdr.g), process(hdr.b) };
    }


    Color32 toneMapToSrgb( const LinearRGB& hdr, std::uint8_t alpha,
                            const ToneMappingSettings& settings ) {
        return encodeSrgb(toneMapLinear(hdr, settings), alpha);
    }
}
