#include "assets/geometry/primitives/CylinderMeshGenerator.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "assets/mesh/Vertex.h"


namespace ve
{
    MeshData CylinderMeshGenerator::create( float radius, float halfHeight, int segments ) {
        if (!std::isfinite(radius) || !std::isfinite(halfHeight) || radius <= 0.0f ||
            halfHeight <= 0.0f || segments < 3 || segments > 4096)
            throw std::invalid_argument("Cylinder dimensions or segments are invalid");

        constexpr float Pi = 3.14159265358979323846f;
        constexpr Color32 White = makeColor(255, 255, 255);
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;
        vertices.reserve(static_cast<std::size_t>(segments + 1) * 6 + 2);
        indices.reserve(static_cast<std::size_t>(segments) * 12);

        // Two seam vertices per ring preserve continuous UVs along the side.
        for (int i = 0; i <= segments; i++) {
            const float u = static_cast<float>(i) / static_cast<float>(segments);
            const float angle = 2.0f * Pi * u;
            const float x = radius * std::cos(angle);
            const float z = radius * std::sin(angle);
            vertices.push_back(Vertex{ Vec3{ x, -halfHeight, z }, White, Vec2{ u, 0.0f } });
            vertices.push_back(Vertex{ Vec3{ x, halfHeight, z }, White, Vec2{ u, 1.0f } });
        }

        for (int i = 0; i < segments; i++) {
            const std::uint32_t base = static_cast<std::uint32_t>(i * 2);
            indices.insert(indices.end(), { base, base + 1, base + 2,
                                             base + 1, base + 3, base + 2 });
        }

        const std::uint32_t bottomCenter = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back(Vertex{ Vec3{ 0.0f, -halfHeight, 0.0f }, White, Vec2{ 0.5f, 0.5f } });
        const std::uint32_t topCenter = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back(Vertex{ Vec3{ 0.0f, halfHeight, 0.0f }, White, Vec2{ 0.5f, 0.5f } });

        const std::uint32_t bottomStart = static_cast<std::uint32_t>(vertices.size());
        for (int i = 0; i < segments; i++) {
            const float a = 2.0f * Pi * static_cast<float>(i) / static_cast<float>(segments);
            const float cx = std::cos(a);
            const float cz = std::sin(a);
            vertices.push_back(Vertex{ Vec3{ radius * cx, -halfHeight, radius * cz }, White,
                                       Vec2{ 0.5f + 0.5f * cx, 0.5f + 0.5f * cz } });
        }
        const std::uint32_t topStart = static_cast<std::uint32_t>(vertices.size());
        for (int i = 0; i < segments; i++) {
            const float a = 2.0f * Pi * static_cast<float>(i) / static_cast<float>(segments);
            const float cx = std::cos(a);
            const float cz = std::sin(a);
            vertices.push_back(Vertex{ Vec3{ radius * cx, halfHeight, radius * cz }, White,
                                       Vec2{ 0.5f + 0.5f * cx, 0.5f + 0.5f * cz } });
        }

        for (int i = 0; i < segments; i++) {
            const std::uint32_t current = static_cast<std::uint32_t>(i);
            const std::uint32_t next = static_cast<std::uint32_t>((i + 1) % segments);
            indices.insert(indices.end(), { bottomCenter, bottomStart + current, bottomStart + next,
                                             topCenter, topStart + next, topStart + current });
        }
        return MeshData(std::move(vertices), std::move(indices));
    }
}
