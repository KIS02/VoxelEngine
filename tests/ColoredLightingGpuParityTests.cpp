#include <array>
#include <cmath>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <glad/gl.h>

#ifndef GLFW_INCLUDE_NONE
    #define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>

#include "assets/material/MaterialData.h"
#include "assets/mesh/MeshData.h"
#include "assets/mesh/Vertex.h"
#include "math/Color.h"
#include "math/Mat4.h"
#include "render/backends/opengl/OpenGLMaterial.h"
#include "render/backends/opengl/OpenGLMesh.h"
#include "render/backends/opengl/OpenGLRenderCommand.h"
#include "render/backends/opengl/OpenGLRenderer.h"
#include "render/backends/opengl/pipeline/shading/DefaultTextureShaders.h"
#include "render/lighting/Lighting.h"


namespace
{
    constexpr int Width = 33;
    constexpr int Height = 33;
    constexpr int Tolerance = 2;


    class HiddenContext {
        private:
            GLFWwindow* window_ = nullptr;

        public:
            HiddenContext() {
                if (glfwInit() != GLFW_TRUE)
                    throw std::runtime_error("GLFW initialization failed");

                glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
                glfwWindowHint(GLFW_SAMPLES, 0);
                window_ = glfwCreateWindow(Width, Height, "Colored Lighting Parity", nullptr, nullptr);

                if (window_ == nullptr) {
                    glfwTerminate();
                    throw std::runtime_error("Hidden GL context creation failed");
                }

                glfwMakeContextCurrent(window_);
            }

            ~HiddenContext() {
                if (window_ != nullptr)
                    glfwDestroyWindow(window_);

                glfwTerminate();
            }

            HiddenContext( const HiddenContext& ) = delete;
            HiddenContext& operator=( const HiddenContext& ) = delete;
    };


    class Target {
        private:
            GLuint framebuffer_ = 0;
            GLuint color_ = 0;
            GLuint depth_ = 0;

        public:
            Target() {
                glGenFramebuffers(1, &framebuffer_);
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

                glGenTextures(1, &color_);
                glBindTexture(GL_TEXTURE_2D, color_);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height,
                             0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_, 0);

                glGenRenderbuffers(1, &depth_);
                glBindRenderbuffer(GL_RENDERBUFFER, depth_);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, Width, Height);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_);

                const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                if (status != GL_FRAMEBUFFER_COMPLETE)
                    throw std::runtime_error("Colored Lighting FBO incomplete");
            }

            ~Target() {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                if (depth_ != 0)
                    glDeleteRenderbuffers(1, &depth_);

                if (color_ != 0)
                    glDeleteTextures(1, &color_);

                if (framebuffer_ != 0)
                    glDeleteFramebuffers(1, &framebuffer_);
            }

            Target( const Target& ) = delete;
            Target& operator=( const Target& ) = delete;

            void clear() const {
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
                glViewport(0, 0, Width, Height);
                glDrawBuffer(GL_COLOR_ATTACHMENT0);
                glReadBuffer(GL_COLOR_ATTACHMENT0);
                glDisable(GL_FRAMEBUFFER_SRGB);
                glDisable(GL_BLEND);
                glDisable(GL_DITHER);
                glDisable(GL_MULTISAMPLE);
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
                glDepthMask(GL_TRUE);
                glEnable(GL_CULL_FACE);
                glFrontFace(GL_CCW);
                glCullFace(GL_BACK);
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClearDepth(1.0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }

            std::array<std::uint8_t, 4> readPixel() const {
                std::array<std::uint8_t, 4> rgba{};
                glReadBuffer(GL_COLOR_ATTACHMENT0);
                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glReadPixels(Width / 2, Height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
                return rgba;
            }
    };


    void testCase( const char* label, ve::OpenGLRenderer& renderer, ve::OpenGLMesh& mesh,
                   ve::OpenGLMaterial& gpuMaterial, const Target& target, const ve::MaterialData& material,
                   const ve::DirectionalLight& light, const ve::Vec3& camera ) {
        target.clear();

        ve::OpenGLDrawCommand command{};
        command.mesh = &mesh;
        command.material = &gpuMaterial;
        command.model = ve::Mat4::identity();
        command.directionalLight = light;
        command.specularStrength = material.specularStrength;
        command.shininess = material.shininess;
        command.diffuseTint = material.diffuseTint;
        command.specularColor = material.specularColor;
        command.lightingDebugMode = 0;

        renderer.submit(command, ve::Mat4::identity(), ve::Mat4::identity(), camera);

        const auto actual = target.readPixel();
        const ve::Color32 expected = ve::shadeColorBlinnPhong(
            ve::makeColor(100, 120, 150, 73), ve::Vec3{ 0.0f, 0.0f, 1.0f },
            ve::Vec3{}, camera, light, material
        );

        const std::array<std::uint8_t, 4> reference{
            ve::getRed(expected), ve::getGreen(expected), ve::getBlue(expected), ve::getAlpha(expected)
        };

        std::cout << "[" << label << "] CPU=";

        for (std::size_t i = 0; i < 4; ++i)
            std::cout << (i == 0 ? "(" : ", ") << static_cast<int>(reference[i]);

        std::cout << ") GPU=";

        for (std::size_t i = 0; i < 4; ++i)
            std::cout << (i == 0 ? "(" : ", ") << static_cast<int>(actual[i]);

        std::cout << ")\n";

        for (std::size_t i = 0; i < 4; ++i) {
            const int delta = std::abs(static_cast<int>(actual[i]) - static_cast<int>(reference[i]));

            if (delta > Tolerance)
                throw std::runtime_error(std::string(label) + ": CPU/GPU channel mismatch");
        }

        if (glGetError() != GL_NO_ERROR)
            throw std::runtime_error(std::string(label) + ": OpenGL error");

        std::cout << "[PASS] " << label << '\n';
    }
}


int main() {
    try {
        HiddenContext context{};
        ve::OpenGLRenderer renderer{};
        renderer.initialize();
        Target target{};

        std::vector<ve::Vertex> vertices(3);
        vertices[0].position = ve::Vec3{ -0.9f, -0.9f, 0.0f };
        vertices[1].position = ve::Vec3{ 0.9f, -0.9f, 0.0f };
        vertices[2].position = ve::Vec3{ 0.0f, 0.9f, 0.0f };

        for (ve::Vertex& vertex : vertices) {
            vertex.normal = ve::Vec3{ 0.0f, 0.0f, 1.0f };
            vertex.uv = ve::Vec2{ 0.5f, 0.5f };
        }

        ve::MeshData meshData(
            std::move(vertices), std::vector<std::uint32_t>{ 0, 1, 2 }
        );

        ve::OpenGLMesh mesh{};
        mesh.initialize(meshData);

        const std::uint8_t rgba[]{ 100, 120, 150, 73 };
        ve::OpenGLMaterial gpuMaterial{};
        gpuMaterial.initialize(
            ve::DefaultTextureVertexShader, ve::DefaultTextureFragmentShader, 1, 1, rgba
        );

        const ve::Vec3 camera{ 0.0f, 0.0f, 5.0f };
        ve::DirectionalLight light{
            ve::Vec3{ 0.0f, 0.0f, 1.0f }, 0.2f, 0.8f
        };

        ve::MaterialData material{};
        material.specularStrength = 0.4f;
        material.shininess = 16.0f;
        testCase("Neutral Specular", renderer, mesh, gpuMaterial, target, material, light, camera);

        material.specularStrength = 0.0f;
        light.color = ve::LinearRGB{ 1.0f, 0.0f, 0.0f };
        testCase("Red Directional Light", renderer, mesh, gpuMaterial, target, material, light, camera);

        material.diffuseTint = ve::LinearRGB{ 0.0f, 1.0f, 1.0f };
        light.color = ve::LinearRGB{ 0.0f, 0.2f, 1.0f };
        testCase("Cyan Material and Blue Light", renderer, mesh, gpuMaterial, target, material, light, camera);

        material.diffuseTint = ve::LinearRGB{ 0.0f, 0.0f, 1.0f };
        material.specularColor = ve::LinearRGB{ 1.0f, 0.0f, 0.0f };
        material.specularStrength = 0.5f;
        light.color = ve::LinearRGB{ 1.0f, 1.0f, 1.0f };
        testCase("Colored Specular", renderer, mesh, gpuMaterial, target, material, light, camera);

        std::cout << "All ColoredLightingGpuParity tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}