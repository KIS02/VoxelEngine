#include "render/opengl/OpenGLRenderer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <type_traits>

#include "math/Mat4.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>



namespace ve
{
    namespace
    {
        struct GPUVertex
        {
            float position[3];
            float color[3];
            float uv[2];
        };


        static_assert(std::is_standard_layout_v<GPUVertex>);
        static_assert(sizeof(GPUVertex) == sizeof(float) * 8);
    }



    OpenGLRenderer::~OpenGLRenderer() {
        if (ebo_ != 0) {
            glDeleteBuffers(1, &ebo_);
        }

        if (vbo_ != 0) {
            glDeleteBuffers(1, &vbo_);
        }

        if (vao_ != 0) {
            glDeleteVertexArrays(1, &vao_);
        }
    }



    void OpenGLRenderer::initialize() {
        if (glfwGetCurrentContext() == nullptr) {
            throw std::runtime_error("No current OpenGL context");
        }


        const int version = gladLoadGL(glfwGetProcAddress);

        if (version == 0) {
            throw std::runtime_error("Failed to initialize GLAD");
        }


        const int major = GLAD_VERSION_MAJOR(version);
        const int minor = GLAD_VERSION_MINOR(version);

        if (major < 3 || (major == 3 && minor < 3)) {
            throw std::runtime_error("OpenGL 3.3 or higher is required");
        }


        std::cout << "[GLAD] OpenGL " << major << "." << minor << " loaded" << std::endl;


        const GLubyte* versionString = glGetString(GL_VERSION);
        const GLubyte* rendererString = glGetString(GL_RENDERER);

        if (versionString != nullptr) {
            std::cout << "[OpenGL] Version: "
                      << reinterpret_cast<const char*>(versionString)
                      << std::endl;
        }

        if (rendererString != nullptr) {
            std::cout << "[OpenGL] Renderer: "
                      << reinterpret_cast<const char*>(rendererString)
                      << std::endl;
        }



        // Vertex Shader

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



        // Fragment Shader

        constexpr const char* fragmentShaderSource = R"(#version 330 core

in vec2 vUV;

uniform sampler2D uTexture;

layout(location = 0) out vec4 FragColor;

void main() {
    FragColor = texture(uTexture, vUV);
}
)";



        // Shader Program

        shader_.initialize(vertexShaderSource, fragmentShaderSource);
        shader_.use();


        GLint currentProgram = 0;

        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

        if (currentProgram != static_cast<GLint>(shader_.id())) {
            throw std::runtime_error("Failed to activate Shader Program");
        }


        std::cout << "[Shader] Program activated successfully" << std::endl;



        // Uniform Locations

        modelLocation_ = glGetUniformLocation(shader_.id(), "uModel");
        viewLocation_ = glGetUniformLocation(shader_.id(), "uView");
        projectionLocation_ = glGetUniformLocation(shader_.id(), "uProjection");
        textureLocation_ = glGetUniformLocation(shader_.id(), "uTexture");


        if (modelLocation_ == -1 || viewLocation_ == -1 ||
            projectionLocation_ == -1 || textureLocation_ == -1) {
            throw std::runtime_error("Failed to find Shader Uniform locations");
        }


        glUniform1i(textureLocation_, 0);


        std::cout << "[OpenGL] MVP and Texture Uniforms initialized" << std::endl;

        glUseProgram(0);



        // Cube Vertices: 6 Faces * 4 Vertices = 24 Vertices

        constexpr std::array<GPUVertex, 24> vertices{
            // Front (+Z)
            GPUVertex{ { -1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
            GPUVertex{ {  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
            GPUVertex{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
            GPUVertex{ { -1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },

            // Back (-Z)
            GPUVertex{ { -1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
            GPUVertex{ { -1.0f,  1.0f, -1.0f }, { 1.0f, 0.5f, 0.0f }, { 1.0f, 0.0f } },
            GPUVertex{ {  1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
            GPUVertex{ {  1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },

            // Left (-X)
            GPUVertex{ { -1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
            GPUVertex{ { -1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
            GPUVertex{ { -1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
            GPUVertex{ { -1.0f,  1.0f, -1.0f }, { 1.0f, 0.5f, 0.0f }, { 0.0f, 1.0f } },

            // Right (+X)
            GPUVertex{ {  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
            GPUVertex{ {  1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
            GPUVertex{ {  1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
            GPUVertex{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },

            // Top (+Y)
            GPUVertex{ { -1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
            GPUVertex{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
            GPUVertex{ {  1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
            GPUVertex{ { -1.0f,  1.0f, -1.0f }, { 1.0f, 0.5f, 0.0f }, { 0.0f, 1.0f } },

            // Bottom (-Y)
            GPUVertex{ { -1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
            GPUVertex{ {  1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
            GPUVertex{ {  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
            GPUVertex{ { -1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } }
        };



        // Cube Indices: 12 Triangles * 3 Indices = 36 Indices

        constexpr std::array<GLuint, 36> indices{
            // Front
            0, 1, 2,
            0, 2, 3,

            // Back
            4, 5, 6,
            4, 6, 7,

            // Left
            8, 9, 10,
            8, 10, 11,

            // Right
            12, 13, 14,
            12, 14, 15,

            // Top
            16, 17, 18,
            16, 18, 19,

            // Bottom
            20, 21, 22,
            20, 22, 23
        };



        // Create VAO / VBO / EBO

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);


        if (vao_ == 0 || vbo_ == 0 || ebo_ == 0) {
            throw std::runtime_error("Failed to create Cube VAO, VBO, or EBO");
        }



        // Bind VAO

        glBindVertexArray(vao_);



        // Upload Vertex Data

        glBindBuffer(GL_ARRAY_BUFFER, vbo_);

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(sizeof(vertices)),
            vertices.data(),
            GL_STATIC_DRAW
        );



        // Upload Index Data

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(sizeof(indices)),
            indices.data(),
            GL_STATIC_DRAW
        );



        // Vertex Attribute Layout

        const GLsizei stride = static_cast<GLsizei>(sizeof(GPUVertex));

        const void* colorOffset =
            reinterpret_cast<const void*>(offsetof(GPUVertex, color));

        const void* uvOffset =
            reinterpret_cast<const void*>(offsetof(GPUVertex, uv));



        // Attribute 0: Position

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
        glEnableVertexAttribArray(0);



        // Attribute 1: Color

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, colorOffset);
        glEnableVertexAttribArray(1);



        // Attribute 2: UV

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, uvOffset);
        glEnableVertexAttribArray(2);



        // Verify UV Attribute

        GLint uvAttributeEnabled = GL_FALSE;
        GLint uvAttributeSize = 0;
        GLint uvAttributeStride = 0;

        glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &uvAttributeEnabled);
        glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_SIZE, &uvAttributeSize);
        glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &uvAttributeStride);


        if (uvAttributeEnabled != GL_TRUE ||
            uvAttributeSize != 2 ||
            uvAttributeStride != stride) {
            throw std::runtime_error("Failed to configure UV Vertex Attribute");
        }


        std::cout << "[OpenGL] UV Attribute configured at location 2" << std::endl;



        // Unbind VAO

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);



        // Depth Buffer Check

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        GLint depthBits = 0;

        glGetFramebufferAttachmentParameteriv(
            GL_FRAMEBUFFER,
            GL_DEPTH,
            GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,
            &depthBits
        );


        if (depthBits <= 0) {
            throw std::runtime_error("OpenGL framebuffer has no depth buffer");
        }


        std::cout << "[OpenGL] Depth Buffer: " << depthBits << " bits" << std::endl;



        // Depth Testing

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glClearDepth(1.0);



        // Back-Face Culling

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);



        // Generate UV Test Texture on CPU

        constexpr int textureWidth = 64;
        constexpr int textureHeight = 64;
        constexpr int cellSize = 8;
        constexpr int markerSize = 16;

        std::array<std::uint8_t, textureWidth * textureHeight * 4> pixels{};


        for (int y = 0; y < textureHeight; ++y) {
            for (int x = 0; x < textureWidth; ++x) {
                const bool isWhite = ((x / cellSize) + (y / cellSize)) % 2 == 0;

                const std::uint8_t checkerColor = isWhite ? 255 : 40;

                std::uint8_t red = checkerColor;
                std::uint8_t green = checkerColor;
                std::uint8_t blue = checkerColor;



                // Bottom-Left: Red

                if (x < markerSize && y < markerSize) {
                    red = 255;
                    green = 0;
                    blue = 0;
                }

                // Bottom-Right: Green

                else if (x >= textureWidth - markerSize && y < markerSize) {
                    red = 0;
                    green = 255;
                    blue = 0;
                }

                // Top-Left: Blue

                else if (x < markerSize && y >= textureHeight - markerSize) {
                    red = 0;
                    green = 0;
                    blue = 255;
                }

                // Top-Right: Yellow

                else if (x >= textureWidth - markerSize && y >= textureHeight - markerSize) {
                    red = 255;
                    green = 255;
                    blue = 0;
                }



                // RGBA Pixel Offset

                const std::size_t offset =
                    static_cast<std::size_t>((y * textureWidth + x) * 4);

                pixels[offset + 0] = red;
                pixels[offset + 1] = green;
                pixels[offset + 2] = blue;
                pixels[offset + 3] = 255;
            }
        }



        // Upload Texture

        texture_.initialize(textureWidth, textureHeight, pixels.data());


        std::cout << "[OpenGL] UV Test Texture uploaded successfully: "
                  << texture_.width() << " x " << texture_.height()
                  << " RGBA8" << std::endl;

        std::cout << "[OpenGL] Texture ID: " << texture_.id() << std::endl;



        std::cout << "[OpenGL] Cube Mesh initialized: "
                  << vertices.size() << " vertices, "
                  << indices.size() << " indices" << std::endl;

        std::cout << "[OpenGL] Depth Testing enabled" << std::endl;
        std::cout << "[OpenGL] Back-Face Culling enabled" << std::endl;
    }



    void OpenGLRenderer::clear( float red, float green, float blue, float alpha ) {
        GLFWwindow* context = glfwGetCurrentContext();

        if (context == nullptr) {
            throw std::runtime_error("No current OpenGL context");
        }


        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(context, &width, &height);

        glViewport(0, 0, width, height);


        glClearColor(red, green, blue, alpha);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }



    void OpenGLRenderer::drawCube( const Mat4& model, const Mat4& view, const Mat4& projection ) {
        if (vao_ == 0 || vbo_ == 0 || ebo_ == 0 || texture_.id() == 0) {
            throw std::runtime_error("OpenGL Cube or Texture is not initialized");
        }



        // Activate Shader Program

        shader_.use();



        // Upload MVP Matrices

        glUniformMatrix4fv(modelLocation_, 1, GL_TRUE, &model.m[0][0]);
        glUniformMatrix4fv(viewLocation_, 1, GL_TRUE, &view.m[0][0]);
        glUniformMatrix4fv(projectionLocation_, 1, GL_TRUE, &projection.m[0][0]);



        // Bind Texture Unit 0

        texture_.bind(0);



        // Draw Cube

        glBindVertexArray(vao_);

        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);



        // Unbind Resources

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }
}