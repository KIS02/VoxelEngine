#include "assets/AssetManager.h"

#include <stdexcept>
#include <utility>


namespace ve
{
    std::uint64_t AssetManager::addMesh( MeshData mesh ) {
        const std::uint64_t id = meshIds_.generate();
        meshes_.emplace(id, std::make_unique<MeshData>(std::move(mesh)));
        return id;
    }

    std::uint64_t AssetManager::addTexture( Texture2D texture ) {
        auto value = std::make_unique<Texture2D>(std::move(texture));
        const std::uint64_t id = value->id();
        textures_.emplace(id, std::move(value));
        return id;
    }

    std::uint64_t AssetManager::addMaterial( std::uint64_t textureId ) {
        if (nextMaterialId_ == 0)
            throw std::overflow_error("Material ID space exhausted");

        const auto textureIt = textures_.find(textureId);
        if (textureIt == textures_.end())
            throw std::out_of_range("Unknown Texture ID");

        const std::uint64_t id = nextMaterialId_++;
        auto material = std::make_unique<MaterialData>();
        material->texture = textureIt->second.get();
        materials_.emplace(id, std::move(material));
        return id;
    }

    const MeshData& AssetManager::mesh( std::uint64_t id ) const {
        return *meshes_.at(id);
    }

    const Texture2D& AssetManager::texture( std::uint64_t id ) const {
        return *textures_.at(id);
    }

    const MaterialData& AssetManager::material( std::uint64_t id ) const {
        return *materials_.at(id);
    }

    MeshIdGenerator& AssetManager::meshIds() {
        return meshIds_;
    }
}
