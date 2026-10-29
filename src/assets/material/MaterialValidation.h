#pragma once

#include <cmath>
#include <stdexcept>

#include "assets/material/MaterialData.h"


namespace ve
{
    inline void validateMaterialProperties( const MaterialData& material ) {
        if (!std::isfinite(material.specularStrength) || material.specularStrength < 0.0f ||
            material.specularStrength > 1.0f || !std::isfinite(material.shininess) ||
            material.shininess <= 0.0f)
            throw std::invalid_argument("Invalid Material specular properties");

        const auto valid = []( float value ) {
            return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
        };

        if (!valid(material.diffuseTint.r) || !valid(material.diffuseTint.g) ||
            !valid(material.diffuseTint.b) || !valid(material.specularColor.r) ||
            !valid(material.specularColor.g) || !valid(material.specularColor.b))
            throw std::invalid_argument("Invalid Material RGB colors");
    }

    inline void validatePbrMaterialProperties( const MaterialData& material ) {
        const auto valid = []( float value ) {
            return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
        };
        if (!valid(material.roughnessFactor) || !valid(material.metallicFactor) ||
            !valid(material.aoStrength) || !valid(material.normalStrength))
            throw std::invalid_argument("Invalid PBR material factors");
    }
}