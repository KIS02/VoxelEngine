#include "render/opengl/OpenGLMesh.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "render/Color.h"
#include "render/MeshData.h"

#include <GLFW/glfw3.h>



namespace ve
{
    namespace
    {
        struct OpenGLVertex
        {
            float position[3];
            float color[3];
            float uv[2];
        };


        static_assert(std::is_standard_layout_v<OpenGLVertex>);
        static_assert(sizeof(OpenGLVertex) == sizeof(float) * 8);
        static_assert(sizeof(std::uint32_t) == sizeof(GLuint));



        constexpr float normalizeColor( std::uint8_t channel ) {
            return static_cast<float>(channel) / 255.0f;
        }
    }



    OpenGLMesh::~OpenGLMesh() {
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



    void OpenGLMesh::initialize( const MeshData& meshData ) {
        if (vao_ != 0 || vbo_ != 0 || ebo_ != 0) {
            throw std::runtime_error("OpenGLMesh is already initialized");
        }

        if (glfwGetCurrentContext() == nullptr) {
            throw std::runtime_error("No current OpenGL context");
        }

        if (meshData.vertexCount() == 0 || meshData.indexCount() == 0) {
            throw std::invalid_argument("Cannot upload an empty Mesh");
        }

        if (meshData.indexCount() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
            throw std::invalid_argument("Mesh index count exceeds OpenGL limit");
        }


        const std::size_t maxBufferSize =
            static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max());


        if (meshData.vertexCount() > maxBufferSize / sizeof(OpenGLVertex) ||
            meshData.indexCount() > maxBufferSize / sizeof(std::uint32_t)) {
            throw std::invalid_argument("Mesh buffer size exceeds OpenGL limit");
        }



        // Convert Engine Vertex to GPU Vertex

        std::vector<OpenGLVertex> gpuVertices;
        gpuVertices.reserve(meshData.vertexCount());


        for (const Vertex& vertex : meshData.vertices()) {
            const float red = normalizeColor(getRed(vertex.color));
            const float green = normalizeColor(getGreen(vertex.color));
            const float blue = normalizeColor(getBlue(vertex.color));


            gpuVertices.push_back(OpenGLVertex{
                { vertex.position.x, vertex.position.y, vertex.position.z },
                { red, green, blue },
                { vertex.uv.x, vertex.uv.y }
            });
        }



        // Create OpenGL Objects

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);


        if (vao_ == 0 || vbo_ == 0 || ebo_ == 0) {
            throw std::runtime_error("Failed to create OpenGL Mesh buffers");
        }



        // Bind Vertex Array

        glBindVertexArray(vao_);



        // Upload Vertex Buffer

        glBindBuffer(GL_ARRAY_BUFFER, vbo_);

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(gpuVertices.size() * sizeof(OpenGLVertex)),
            gpuVertices.data(),
            GL_STATIC_DRAW
        );



        // Upload Index Buffer

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(meshData.indexCount() * sizeof(std::uint32_t)),
            meshData.indices().data(),
            GL_STATIC_DRAW
        );



        // Vertex Attribute Layout

        const GLsizei stride = static_cast<GLsizei>(sizeof(OpenGLVertex));

        const void* colorOffset =
            reinterpret_cast<const void*>(offsetof(OpenGLVertex, color));

        const void* uvOffset =
            reinterpret_cast<const void*>(offsetof(OpenGLVertex, uv));



        // Attribute 0: Position

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
        glEnableVertexAttribArray(0);



        // Attribute 1: Color

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, colorOffset);
        glEnableVertexAttribArray(1);



        // Attribute 2: Texture Coordinates

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, uvOffset);
        glEnableVertexAttribArray(2);



        // Verify UV Attribute

        GLint uvEnabled = GL_FALSE;
        GLint uvSize = 0;
        GLint uvStride = 0;

        glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &uvEnabled);
        glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_SIZE, &uvSize);
        glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &uvStride);


        if (uvEnabled != GL_TRUE || uvSize != 2 || uvStride != stride) {
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            throw std::runtime_error("Failed to configure OpenGL Mesh UV Attribute");
        }



        // Unbind Resources

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);



        // Store Draw Information

        indexCount_ = static_cast<GLsizei>(meshData.indexCount());
    }



    void OpenGLMesh::draw() const {
        if (vao_ == 0 || indexCount_ == 0) {
            throw std::runtime_error("OpenGLMesh is not initialized");
        }


        glBindVertexArray(vao_);

        glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);

        glBindVertexArray(0);
    }



    GLsizei OpenGLMesh::indexCount() const {
        return indexCount_;
    }
}