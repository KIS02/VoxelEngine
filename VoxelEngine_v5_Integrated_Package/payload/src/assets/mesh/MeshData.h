#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "assets/mesh/Vertex.h"



namespace ve
{
    class MeshData {
        private:
            std::vector<Vertex> vertices_;
            std::vector<std::uint32_t> indices_;


        public:
            MeshData( std::vector<Vertex> vertices, std::vector<std::uint32_t> indices );


            const std::vector<Vertex>& vertices() const;
            const std::vector<std::uint32_t>& indices() const;


            std::size_t vertexCount() const;
            std::size_t indexCount() const;
            std::size_t triangleCount() const;
    };
}