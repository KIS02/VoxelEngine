#pragma once

#include "render/opengl/OpenGLMaterial.h"
#include "render/opengl/OpenGLMesh.h"



namespace ve
{
    class MeshData;
    class Texture2D;


    class OpenGLCubeDemo {
        private:
            OpenGLMaterial material_;
            OpenGLMesh mesh_;


        public:
            void initialize( const MeshData& meshData, const Texture2D& texture );

            const OpenGLMesh& mesh() const;
            OpenGLMaterial& material();
    };
}