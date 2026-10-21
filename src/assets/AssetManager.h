#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include "assets/material/MaterialData.h"
#include "assets/mesh/MeshData.h"
#include "assets/mesh/MeshIdGenerator.h"
#include "assets/texture/Texture2D.h"


namespace ve
{
    // CPU-side resources outlive every Scene and every RenderFrame referencing them.
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
            std::uint64_t addMaterial( std::uint64_t textureId );
            const MeshData& mesh( std::uint64_t id ) const;
            const Texture2D& texture( std::uint64_t id ) const;
            const MaterialData& material( std::uint64_t id ) const;
            MeshIdGenerator& meshIds();
    };
}
