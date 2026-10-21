#include "geometry/CubeMeshFactory.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include "math/Color.h"
#include "assets/mesh/Vertex.h"



namespace ve
{
    namespace
    {
        void appendFace( std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices,
                         const std::array<Vec3, 4>& positions, const std::array<Color32, 4>& colors ) {

            const std::uint32_t baseIndex = static_cast<std::uint32_t>(vertices.size());


            vertices.push_back(Vertex{ positions[0], colors[0], Vec2{ 0.0f, 0.0f } });
            vertices.push_back(Vertex{ positions[1], colors[1], Vec2{ 1.0f, 0.0f } });
            vertices.push_back(Vertex{ positions[2], colors[2], Vec2{ 1.0f, 1.0f } });
            vertices.push_back(Vertex{ positions[3], colors[3], Vec2{ 0.0f, 1.0f } });


            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);

            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 3);
        }
    }



    MeshData CubeMeshFactory::create( float halfExtent ) {
        if (!std::isfinite(halfExtent) || halfExtent <= 0.0f) {
            throw std::invalid_argument("Cube half extent must be finite and positive");
        }


        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;

        vertices.reserve(24);
        indices.reserve(36);


        const float n = -halfExtent;
        const float p = halfExtent;



        // Cube Corner Colors

        const Color32 red = makeColor(255, 0, 0);
        const Color32 green = makeColor(0, 255, 0);
        const Color32 blue = makeColor(0, 0, 255);
        const Color32 yellow = makeColor(255, 255, 0);

        const Color32 magenta = makeColor(255, 0, 255);
        const Color32 cyan = makeColor(0, 255, 255);
        const Color32 white = makeColor(255, 255, 255);
        const Color32 orange = makeColor(255, 128, 0);



        // Front Face (+Z)

        appendFace(
            vertices, indices,
            std::array<Vec3, 4>{
                Vec3{ n, n, p }, Vec3{ p, n, p },
                Vec3{ p, p, p }, Vec3{ n, p, p }
            },
            std::array<Color32, 4>{ red, green, blue, yellow }
        );



        // Back Face (-Z)

        appendFace(
            vertices, indices,
            std::array<Vec3, 4>{
                Vec3{ n, n, n }, Vec3{ n, p, n },
                Vec3{ p, p, n }, Vec3{ p, n, n }
            },
            std::array<Color32, 4>{ magenta, orange, white, cyan }
        );



        // Left Face (-X)

        appendFace(
            vertices, indices,
            std::array<Vec3, 4>{
                Vec3{ n, n, n }, Vec3{ n, n, p },
                Vec3{ n, p, p }, Vec3{ n, p, n }
            },
            std::array<Color32, 4>{ magenta, red, yellow, orange }
        );



        // Right Face (+X)

        appendFace(
            vertices, indices,
            std::array<Vec3, 4>{
                Vec3{ p, n, p }, Vec3{ p, n, n },
                Vec3{ p, p, n }, Vec3{ p, p, p }
            },
            std::array<Color32, 4>{ green, cyan, white, blue }
        );



        // Top Face (+Y)

        appendFace(
            vertices, indices,
            std::array<Vec3, 4>{
                Vec3{ n, p, p }, Vec3{ p, p, p },
                Vec3{ p, p, n }, Vec3{ n, p, n }
            },
            std::array<Color32, 4>{ yellow, blue, white, orange }
        );



        // Bottom Face (-Y)

        appendFace(
            vertices, indices,
            std::array<Vec3, 4>{
                Vec3{ n, n, n }, Vec3{ p, n, n },
                Vec3{ p, n, p }, Vec3{ n, n, p }
            },
            std::array<Color32, 4>{ magenta, cyan, green, red }
        );


        return MeshData{ std::move(vertices), std::move(indices) };
    }
}