#include "assets/AssetManager.h"

#include <cmath>
#include <stdexcept>
#include <utility>

#include "assets/material/MaterialValidation.h"
#include "assets/texture/ImageLoader.h"


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


    std::uint64_t AssetManager::loadTexture( const std::filesystem::path& path,
                                              TextureColorSpace colorSpace ) {
        return addTexture(Texture2D(ImageLoader::load(path, colorSpace)));
    }


    std::uint64_t AssetManager::addMaterial( std::uint64_t textureId, float specularStrength,
                                             float shininess, const LinearRGB& diffuseTint,
                                             const LinearRGB& specularColor ) {
        MaterialData properties{};
        properties.specularStrength = specularStrength;
        properties.shininess = shininess;
        properties.diffuseTint = diffuseTint;
        properties.specularColor = specularColor;

        validateMaterialProperties(properties);

        if (nextMaterialId_ == 0)
            throw std::overflow_error("Material ID space exhausted");

        const auto textureIt = textures_.find(textureId);

        if (textureIt == textures_.end())
            throw std::out_of_range("Unknown Texture ID");

        const std::uint64_t id = nextMaterialId_++;
        auto material = std::make_unique<MaterialData>(properties);

        material->texture = textureIt->second.get();
        materials_.emplace(id, std::move(material));

        return id;
    }


    void AssetManager::configurePbrMaterial( std::uint64_t materialId, std::uint64_t normalTextureId,
                                              std::uint64_t roughnessTextureId, std::uint64_t metallicTextureId,
                                              std::uint64_t aoTextureId, float roughnessFactor,
                                              float metallicFactor, float aoStrength, float normalStrength ) {
        MaterialData updated = *materials_.at(materialId);
        updated.roughnessFactor = roughnessFactor;
        updated.metallicFactor = metallicFactor;
        updated.aoStrength = aoStrength;
        updated.normalStrength = normalStrength;

        const auto optionalMap = [this]( std::uint64_t textureId ) -> const Texture2D* {
            if (textureId == 0)
                return nullptr;
            const Texture2D& resolved = *textures_.at(textureId);
            if (resolved.colorSpace() != TextureColorSpace::Linear)
                throw std::invalid_argument("PBR data maps must use Linear colorspace");
            return &resolved;
        };

        updated.normalMap = optionalMap(normalTextureId);
        updated.roughnessMap = optionalMap(roughnessTextureId);
        updated.metallicMap = optionalMap(metallicTextureId);
        updated.aoMap = optionalMap(aoTextureId);
        validatePbrMaterialProperties(updated);
        *materials_.at(materialId) = updated;
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