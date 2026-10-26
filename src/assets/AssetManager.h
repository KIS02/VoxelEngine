#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>

#include "assets/material/MaterialData.h"
#include "assets/mesh/MeshData.h"
#include "assets/mesh/MeshIdGenerator.h"
#include "assets/texture/Texture2D.h"


namespace ve
{
    class AssetManager {
        private:
            MeshIdGenerator meshIds_;
            std::map<std::uint64_t, std::unique_ptr<MeshData>> meshes_;
            std::map<std::uint64_t, std::unique_ptr<Texture2D>> textures_;
            std::map<std::uint64_t, std::unique_ptr<MaterialData>> materials_;
            std::uint64_t nextMaterialId_ = 1;


        public:
            std::uint64_t addMesh( MeshData mesh );
            std::uint64_t addTexture( Texture2D texture );

            std::uint64_t loadTexture( const std::filesystem::path& path,
                                       TextureColorSpace colorSpace = TextureColorSpace::SRGB );

            std::uint64_t addMaterial( std::uint64_t textureId, float specularStrength = 0.35f,
                                       float shininess = 32.0f,
                                       const LinearRGB& diffuseTint = LinearRGB{ 1.0f, 1.0f, 1.0f },
                                       const LinearRGB& specularColor = LinearRGB{ 1.0f, 1.0f, 1.0f } );

            const MeshData& mesh( std::uint64_t id ) const;
            const Texture2D& texture( std::uint64_t id ) const;
            const MaterialData& material( std::uint64_t id ) const;

            MeshIdGenerator& meshIds();
    };
}