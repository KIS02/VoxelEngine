#include "voxel/generation/FractalNoise2D.h"

#include <cmath>
#include <stdexcept>

#include "voxel/generation/ValueNoise2D.h"


namespace ve
{
    double fractalNoise2D( WorldSeed seed, double worldX, double worldZ,
                           const FractalNoiseConfig& config ) {
        if (config.octaves < 1 || config.octaves > 12)
            throw std::invalid_argument("Octaves must be between 1 and 12");

        if (!std::isfinite(config.frequency) || config.frequency <= 0.0)
            throw std::invalid_argument("Frequency must be positive and finite");

        if (!std::isfinite(config.lacunarity) || config.lacunarity < 1.0)
            throw std::invalid_argument("Lacunarity must be at least 1 and finite");

        if (!std::isfinite(config.persistence) || config.persistence <= 0.0 || config.persistence > 1.0)
            throw std::invalid_argument("Persistence must be in (0, 1]");


        double frequency = config.frequency;
        double amplitude = 1.0;
        double weightedSum = 0.0;
        double totalAmplitude = 0.0;

        for (int octave = 0; octave < config.octaves; ++octave) {
            const double value = valueNoise2D(seed, worldX * frequency, worldZ * frequency);

            weightedSum += value * amplitude;
            totalAmplitude += amplitude;

            frequency *= config.lacunarity;
            amplitude *= config.persistence;
        }

        return weightedSum / totalAmplitude;
    }
}