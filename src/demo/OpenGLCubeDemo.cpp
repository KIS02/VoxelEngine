#include "demo/OpenGLCubeDemo.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "render/Color.h"
#include "render/MeshData.h"
#include "render/Texture2D.h"



namespace ve
{
    namespace
    {
        constexpr const char* vertexShaderSource = R"(#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vUV;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
    vUV = aUV;
}
)";



        constexpr const char* fragmentShaderSource = R"(#version 330 core

in vec2 vUV;

uniform sampler2D uTexture;
uniform vec4 uTint;

layout(location = 0) out vec4 FragColor;

void main() {
    vec4 textureColor = texture(uTexture, vUV);
    FragColor = textureColor * uTint;
}
)";



        std::vector<std::uint8_t> createRGBA8Pixels( const Texture2D& texture ) {
            const int width = texture.width();
            const int height = texture.height();


            if (width <= 0 || height <= 0) {
                throw std::invalid_argument("Invalid source Texture dimensions");
            }



            // Allocate Explicit RGBA8 Byte Buffer

            const std::size_t pixelCount =
                static_cast<std::size_t>(width) * static_cast<std::size_t>(height);

            std::vector<std::uint8_t> pixels(pixelCount * 4);



            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    const Color32 color = texture.pixel(x, y);

                    const std::size_t index =
                        static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                        static_cast<std::size_t>(x);

                    const std::size_t offset = index * 4;



                    // Convert 0xAARRGGBB to RGBA8 Byte Order

                    pixels[offset + 0] = getRed(color);
                    pixels[offset + 1] = getGreen(color);
                    pixels[offset + 2] = getBlue(color);
                    pixels[offset + 3] = getAlpha(color);
                }
            }


            return pixels;
        }
    }



    void OpenGLCubeDemo::initialize( const MeshData& meshData, const Texture2D& texture ) {
        // Upload Shared MeshData

        mesh_.initialize(meshData);


        std::cout << "[CubeDemo] Shared MeshData uploaded: "
                  << meshData.vertexCount() << " vertices, "
                  << mesh_.indexCount() << " indices"
                  << std::endl;



        // Convert Shared Texture2D to RGBA8

        const std::vector<std::uint8_t> pixels = createRGBA8Pixels(texture);



        // Initialize Material Using the Same Texture Data

        material_.initialize(
            vertexShaderSource,
            fragmentShaderSource,
            texture.width(),
            texture.height(),
            pixels.data()
        );


        material_.setTint(makeColor(255, 255, 255, 255));


        std::cout << "[CubeDemo] Material initialized" << std::endl;

        std::cout << "[CubeDemo] Shared Texture uploaded: "
                  << texture.width() << " x " << texture.height()
                  << " RGBA8"
                  << std::endl;
    }



    const OpenGLMesh& OpenGLCubeDemo::mesh() const {
        return mesh_;
    }



    OpenGLMaterial& OpenGLCubeDemo::material() {
        return material_;
    }
}