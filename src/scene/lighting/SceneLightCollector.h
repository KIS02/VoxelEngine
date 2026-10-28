#pragma once

#include <cmath>
#include <stdexcept>
#include <utility>

#include "math/Mat4.h"
#include "math/Vec4.h"
#include "render/lighting/Lighting.h"
#include "scene/object/SceneObjectRegistry.h"


namespace ve
{
    class SceneLightCollector {
        private:
            static constexpr float RadiansPerDegree = 3.14159265358979323846f / 180.0f;


            static bool finite( const Vec3& value ) {
                return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
            }


            static bool validColor( const LinearRGB& color ) {
                return std::isfinite(color.r) && std::isfinite(color.g) && std::isfinite(color.b) &&
                       color.r >= 0.0f && color.g >= 0.0f && color.b >= 0.0f &&
                       color.r <= 1.0f && color.g <= 1.0f && color.b <= 1.0f;
            }


            static void validate( const SceneObject& object, const LightComponent& light ) {
                if (!finite(object.position) || !finite(object.rotationDegrees))
                    throw std::invalid_argument("Scene Light position/rotation must be finite");

                if (!validColor(light.color) || !std::isfinite(light.intensity) || light.intensity < 0.0f ||
                    !std::isfinite(light.ambient) || light.ambient < 0.0f || light.ambient > 1.0f ||
                    !std::isfinite(light.diffuse) || light.diffuse < 0.0f || light.diffuse > 1.0f)
                    throw std::invalid_argument("Scene Light color/intensity is invalid");

                if (light.type != LightType::Directional && light.type != LightType::Point &&
                    light.type != LightType::Spot)
                    throw std::invalid_argument("Unknown Scene Light type");

                if (light.type == LightType::Point || light.type == LightType::Spot) {
                    if (!std::isfinite(light.range) || light.range <= 0.0f)
                        throw std::invalid_argument("Point/Spot Light range must be positive");
                }

                if (light.type == LightType::Spot) {
                    if (!std::isfinite(light.innerConeDegrees) || !std::isfinite(light.outerConeDegrees) ||
                        light.innerConeDegrees < 0.0f || light.outerConeDegrees >= 90.0f ||
                        light.innerConeDegrees >= light.outerConeDegrees)
                        throw std::invalid_argument("Spot Light cone angles are invalid");
                }
            }


            static Vec3 rotatedEmission( const Vec3& rotationDegrees ) {
                const Vec3 angles = rotationDegrees * RadiansPerDegree;

                const Mat4 rotation = Mat4::rotationZ(angles.z) * Mat4::rotationY(angles.y) *
                                      Mat4::rotationX(angles.x);

                const Vec4 world = rotation * Vec4{ 0.0f, 0.0f, -1.0f, 0.0f };
                const Vec3 result{ world.x, world.y, world.z };

                if (!finite(result) || result.lengthSquared() <= 1.0e-12f)
                    throw std::invalid_argument("Scene Light direction is invalid");

                return result.normalized();
            }


            static SceneLightData makeSnapshot( const SceneObject& object, const LightComponent& light ) {
                validate(object, light);

                SceneLightData result{};
                result.objectId = object.id.value;
                result.type = light.type;
                result.position = object.position;

                result.emissionDirection = rotatedEmission(object.rotationDegrees);
                result.directionToLight = -result.emissionDirection;

                result.color = light.color;
                result.intensity = light.intensity;
                result.ambient = light.ambient;
                result.diffuse = light.diffuse;

                result.range = light.range;
                result.innerConeDegrees = light.innerConeDegrees;
                result.outerConeDegrees = light.outerConeDegrees;
                result.castsShadows = light.castsShadows;

                return result;
            }


        public:
            static void collect( const SceneObjectRegistry& objects, LightingSettings& output ) {
                LightingSettings next{};

                next.directional.ambient = 0.0f;
                next.directional.diffuse = 0.0f;

                bool foundDirectional = false;

                objects.forEach([&next, &foundDirectional]( const SceneObject& object ) {
                    if (!object.light.has_value() || !object.light->enabled)
                        return;

                    const SceneLightData light = makeSnapshot(object, *object.light);

                    next.sceneLights.push_back(light);

                    if (light.type == LightType::Directional && !foundDirectional) {
                        next.directional.directionToLight = light.directionToLight;
                        next.directional.color = light.color;
                        next.directional.ambient = light.ambient;
                        next.directional.diffuse = light.diffuse * light.intensity;

                        foundDirectional = true;
                    }
                });

                output = std::move(next);
            }
    };
}