#pragma once

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

#include "math/Vec3.h"


namespace ve
{
    struct CollisionAabb
    {
        Vec3 min{};
        Vec3 max{};


        void validate() const {
            if (!std::isfinite(min.x) || !std::isfinite(min.y) || !std::isfinite(min.z) ||
                !std::isfinite(max.x) || !std::isfinite(max.y) || !std::isfinite(max.z) ||
                min.x >= max.x || min.y >= max.y || min.z >= max.z)
                throw std::invalid_argument("Invalid collision AABB");
        }

        CollisionAabb translated( const Vec3& displacement ) const {
            return CollisionAabb{ min + displacement, max + displacement };
        }

        bool overlaps( const CollisionAabb& other ) const {
            return min.x < other.max.x && max.x > other.min.x &&
                   min.y < other.max.y && max.y > other.min.y &&
                   min.z < other.max.z && max.z > other.min.z;
        }
    };


    struct SweepContact
    {
        float fraction = 1.0f;
        Vec3 normal{};
    };


    inline std::optional<SweepContact> sweepAabb( const CollisionAabb& moving,
                                                   const CollisionAabb& target, const Vec3& displacement ) {
        if (moving.overlaps(target))
            return std::nullopt; // Existing penetration is handled by spawn/teleport validation.

        float first = 0.0f;
        float last = 1.0f;
        Vec3 normal{};

        for (int axis = 0; axis < 3; ++axis) {
            const float start = axis == 0 ? moving.min.x : (axis == 1 ? moving.min.y : moving.min.z);
            const float end = axis == 0 ? moving.max.x : (axis == 1 ? moving.max.y : moving.max.z);
            const float wallMin = axis == 0 ? target.min.x : (axis == 1 ? target.min.y : target.min.z);
            const float wallMax = axis == 0 ? target.max.x : (axis == 1 ? target.max.y : target.max.z);
            const float delta = axis == 0 ? displacement.x : (axis == 1 ? displacement.y : displacement.z);

            if (delta == 0.0f) {
                if (end <= wallMin || start >= wallMax)
                    return std::nullopt;
                continue;
            }

            float enter = (wallMin - end) / delta;
            float leave = (wallMax - start) / delta;
            Vec3 axisNormal{};
            if (axis == 0)
                axisNormal.x = -1.0f;
            else if (axis == 1)
                axisNormal.y = -1.0f;
            else
                axisNormal.z = -1.0f;

            if (delta < 0.0f) {
                std::swap(enter, leave);
                axisNormal = -axisNormal;
            }

            if (enter > first) {
                first = enter;
                normal = axisNormal;
            } else if (enter == first && normal.lengthSquared() == 0.0f) {
                normal = axisNormal;
            }

            last = std::min(last, leave);
            if (first > last)
                return std::nullopt;
        }

        if (normal.lengthSquared() == 0.0f || first > 1.0f || last < 0.0f)
            return std::nullopt;

        return SweepContact{ std::clamp(first, 0.0f, 1.0f), normal };
    }
}
