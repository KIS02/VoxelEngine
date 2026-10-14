#include "render/MeshData.h"

#include <stdexcept>
#include <utility>



namespace ve
{
    MeshData::MeshData( std::vector<Vertex> vertices, std::vector<std::uint32_t> indices )
        : vertices_(std::move(vertices)), indices_(std::move(indices)) {

        if (indices_.size() % 3 != 0) {
            throw std::invalid_argument("Mesh index count must be a multiple of 3");
        }


        for (const std::uint32_t index : indices_) {
            if (static_cast<std::size_t>(index) >= vertices_.size()) {
                throw std::invalid_argument("Mesh index is out of vertex range");
            }
        }
    }



    const std::vector<Vertex>& MeshData::vertices() const {
        return vertices_;
    }


    const std::vector<std::uint32_t>& MeshData::indices() const {
        return indices_;
    }



    std::size_t MeshData::vertexCount() const {
        return vertices_.size();
    }


    std::size_t MeshData::indexCount() const {
        return indices_.size();
    }


    std::size_t MeshData::triangleCount() const {
        return indices_.size() / 3;
    }
}