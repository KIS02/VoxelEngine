#include "render/backends/opengl/pipeline/shadow/OpenGLShadowPass.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <stdexcept>
#include <string>
#include <vector>

#include "assets/mesh/MeshData.h"
#include "render/backends/opengl/OpenGLMesh.h"
#include "render/backends/opengl/resources/OpenGLResourceCache.h"
#include "render/frame/RenderFrame.h"
#include "render/lighting/Lighting.h"
#include "render/shadow/ShadowFrameFingerprint.h"


namespace ve
{
    namespace
    {
        constexpr int ShadowTextureUnit = 6;
        constexpr int MaxSceneUniformLights = 16;
        constexpr int MaxShadowMatrixUniforms = 24;
        constexpr int DefaultShadowResolution = 1024;

        // Cache ownership is linked to OpenGLShadowPass lifetime in restoreAndRelease().
        // OpenGL work and destruction must occur on the owning GL context/thread.
        std::unordered_map<const OpenGLShadowPass*, std::uint64_t> cachedShadowKeys;

        int shadowResolution() {
            const char* quality = std::getenv("VOXELENGINE_SHADOW_QUALITY");
            if (quality != nullptr && std::strcmp(quality, "medium") == 0)
                return 512;
            if (quality != nullptr && std::strcmp(quality, "low") == 0)
                return 256;
            return DefaultShadowResolution;
        }

        const char* DepthVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
uniform mat4 uShadowModelViewProjection;
void main() {
    gl_Position = uShadowModelViewProjection * vec4(aPosition, 1.0);
}
)";

        const char* DepthFragmentShader = R"(#version 330 core
void main() { }
)";

        GLuint compile( GLenum kind, const char* source ) {
            const GLuint shader = glCreateShader(kind);
            if (shader == 0)
                throw std::runtime_error("Cannot create Shadow shader");

            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);
            GLint status = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
            if (status != GL_TRUE) {
                glDeleteShader(shader);
                throw std::runtime_error("Shadow shader compilation failed");
            }
            return shader;
        }

        GLuint createProgram() {
            const GLuint vertex = compile(GL_VERTEX_SHADER, DepthVertexShader);
            GLuint fragment = 0;
            GLuint program = 0;
            try {
                fragment = compile(GL_FRAGMENT_SHADER, DepthFragmentShader);
                program = glCreateProgram();
                if (program == 0)
                    throw std::runtime_error("Cannot create Shadow program");
                glAttachShader(program, vertex);
                glAttachShader(program, fragment);
                glLinkProgram(program);
                GLint status = GL_FALSE;
                glGetProgramiv(program, GL_LINK_STATUS, &status);
                if (status != GL_TRUE)
                    throw std::runtime_error("Shadow shader linking failed");
            } catch (...) {
                if (program != 0)
                    glDeleteProgram(program);
                if (fragment != 0)
                    glDeleteShader(fragment);
                glDeleteShader(vertex);
                throw;
            }
            glDeleteShader(fragment);
            glDeleteShader(vertex);
            return program;
        }
    }


    OpenGLShadowPass::~OpenGLShadowPass() {
        restoreAndRelease();
    }


    void OpenGLShadowPass::restoreAndRelease() {
        cachedShadowKeys.erase(this);
        if (framebuffer_ != 0)
            glDeleteFramebuffers(1, &framebuffer_);
        if (depthArray_ != 0)
            glDeleteTextures(1, &depthArray_);
        if (program_ != 0)
            glDeleteProgram(program_);
        framebuffer_ = 0;
        depthArray_ = 0;
        program_ = 0;
        resolution_ = 0;
    }


    void OpenGLShadowPass::ensureResources( int resolution ) {
        if (framebuffer_ != 0 && depthArray_ != 0 && program_ != 0 && resolution_ == resolution)
            return;

        restoreAndRelease();
        program_ = createProgram();
        glGenFramebuffers(1, &framebuffer_);
        glGenTextures(1, &depthArray_);
        if (framebuffer_ == 0 || depthArray_ == 0)
            throw std::runtime_error("Cannot allocate Shadow Map resources");

        GLint previousActiveTexture = GL_TEXTURE0;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
        glActiveTexture(GL_TEXTURE0 + ShadowTextureUnit);
        glBindTexture(GL_TEXTURE_2D_ARRAY, depthArray_);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24,
                     resolution, resolution, MaxLayers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        const GLfloat border[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        glActiveTexture(static_cast<GLenum>(previousActiveTexture));
        resolution_ = resolution;
    }


    void OpenGLShadowPass::render( const RenderFrame& frame, OpenGLResourceCache& resources,
                                    BenchmarkRenderStatistics* statistics ) {
        if (frame.lighting.sceneLights.size() > MaxSceneUniformLights)
            throw std::invalid_argument("RenderFrame exceeds 16 Scene Lights");

        std::vector<ShadowLightInput> inputs;
        inputs.reserve(frame.lighting.sceneLights.size());
        for (std::size_t i = 0; i < frame.lighting.sceneLights.size(); ++i) {
            const SceneLightData& light = frame.lighting.sceneLights[i];
            ShadowLightInput input{};
            input.objectId = light.objectId;
            input.type = static_cast<ShadowLightKind>(light.type);
            input.position = light.position;
            input.emissionDirection = light.emissionDirection;
            input.range = light.range;
            input.outerConeDegrees = light.outerConeDegrees;
            input.castsShadows = light.castsShadows;
            inputs.push_back(input);
        }

        ShadowSettings settings{};
        settings.resolution = shadowResolution();
        settings.pcfRadius = 1;

        const std::uint64_t fingerprint = ShadowFrameFingerprint::calculate(frame, settings);
        const auto cached = cachedShadowKeys.find(this);
        if (cached != cachedShadowKeys.end() && cached->second == fingerprint &&
            active() && resolution_ == settings.resolution)
            return;

        baseLayers_.fill(-1);
        prepared_ = ShadowFrame{};

        prepared_ = ShadowMapping::prepare(inputs, frame.cameraPosition, settings);
        if (prepared_.lights.empty()) {
            cachedShadowKeys.erase(this);
            return;
        }
        if (prepared_.lights.size() > MaxCasters)
            throw std::runtime_error("OpenGL supports at most four Shadow-casting Lights");

        int caster = 0;
        for (std::size_t light = 0; light < inputs.size(); ++light) {
            if (inputs[light].castsShadows) {
                baseLayers_[light] = caster * LayersPerCaster;
                ++caster;
            }
        }

        ensureResources(settings.resolution);

        GLint previousFbo = 0;
        GLint previousProgram = 0;
        GLint viewport[4]{};
        GLint previousActiveTexture = GL_TEXTURE0;
        GLint previousDepthFunction = GL_LESS;
        GLboolean previousDepthWrite = GL_TRUE;
        const GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
        const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
        const GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
        const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        const GLboolean polygonOffsetEnabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);
        GLfloat previousOffsetFactor = 0.0f;
        GLfloat previousOffsetUnits = 0.0f;
        glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &previousOffsetFactor);
        glGetFloatv(GL_POLYGON_OFFSET_UNITS, &previousOffsetUnits);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
        glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunction);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthWrite);

        const auto restore = [&]() {
            glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFbo));
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            glUseProgram(static_cast<GLuint>(previousProgram));
            glDepthMask(previousDepthWrite);
            glDepthFunc(static_cast<GLenum>(previousDepthFunction));
            if (depthTestEnabled)
                glEnable(GL_DEPTH_TEST);
            else
                glDisable(GL_DEPTH_TEST);
            if (cullEnabled)
                glEnable(GL_CULL_FACE);
            else
                glDisable(GL_CULL_FACE);
            if (scissorEnabled)
                glEnable(GL_SCISSOR_TEST);
            else
                glDisable(GL_SCISSOR_TEST);
            if (blendEnabled)
                glEnable(GL_BLEND);
            else
                glDisable(GL_BLEND);
            glPolygonOffset(previousOffsetFactor, previousOffsetUnits);
            if (polygonOffsetEnabled)
                glEnable(GL_POLYGON_OFFSET_FILL);
            else
                glDisable(GL_POLYGON_OFFSET_FILL);
            glActiveTexture(static_cast<GLenum>(previousActiveTexture));
        };

        try {
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
            glViewport(0, 0, settings.resolution, settings.resolution);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
            glDisable(GL_CULL_FACE);
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_BLEND);
            // Apply depth bias while rendering shadow casters, not during camera color rendering.
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.5f);
            glUseProgram(program_);
            const GLint matrixLocation = glGetUniformLocation(program_, "uShadowModelViewProjection");
            if (matrixLocation < 0)
                throw std::runtime_error("Shadow shader matrix Uniform missing");

            for (std::size_t light = 0; light < prepared_.lights.size(); ++light) {
                const auto& shadowLight = prepared_.lights[light];
                for (std::size_t face = 0; face < shadowLight.faces.size(); ++face) {
                    const GLuint layer = static_cast<GLuint>(light * LayersPerCaster + face);
                    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthArray_, 0,
                                              static_cast<GLint>(layer));
                    glDrawBuffer(GL_NONE);
                    glReadBuffer(GL_NONE);
                    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                        throw std::runtime_error("Shadow FBO is incomplete");
                    glClearDepth(1.0);
                    glClear(GL_DEPTH_BUFFER_BIT);

                    for (const RenderDraw& draw : frame.draws) {
                        if (draw.mesh == nullptr || draw.mesh->indexCount() == 0)
                            continue;
                        const OpenGLMesh& mesh = resources.resolveMesh(
                            *draw.mesh, draw.meshId, draw.meshRevision, statistics
                        );
                        const Mat4 mvp = shadowLight.faces[face].worldToClip * draw.model;
                        glUniformMatrix4fv(matrixLocation, 1, GL_TRUE, &mvp.m[0][0]);
                        mesh.draw();
                    }
                }
            }
        } catch (...) {
            restore();
            cachedShadowKeys.erase(this);
            throw;
        }
        restore();
        cachedShadowKeys[this] = fingerprint;
    }


    void OpenGLShadowPass::bindForLighting( GLuint programId ) const {
        if (programId == 0)
            throw std::invalid_argument("No lighting Shader Program");

        const auto uniform = [programId]( const char* name ) {
            const GLint location = glGetUniformLocation(programId, name);
            if (location < 0)
                throw std::runtime_error(std::string("Missing Shadow Uniform: ") + name);
            return location;
        };
        glUniform1i(uniform("uShadowMap"), ShadowTextureUnit);
        glUniform1iv(uniform("uShadowBaseLayers[0]"), MaxSceneUniformLights, baseLayers_.data());
        const bool hasShadows = active();
        glUniform1i(uniform("uUseShadows"), hasShadows ? 1 : 0);
        if (!hasShadows)
            return;

        std::array<GLfloat, MaxShadowMatrixUniforms * 16> matrices{};
        for (std::size_t i = 0; i < prepared_.lights.size(); ++i) {
            const auto& light = prepared_.lights[i];
            for (std::size_t j = 0; j < light.faces.size(); ++j) {
                const std::size_t offset = (i * LayersPerCaster + j) * 16;
                const Mat4& matrix = light.faces[j].worldToClip;
                for (int row = 0; row < 4; ++row) {
                    for (int col = 0; col < 4; ++col)
                        matrices[offset + static_cast<std::size_t>(row * 4 + col)] = matrix(row, col);
                }
            }
        }
        const GLint matricesLocation = uniform("uShadowMatrices[0]");
        glUniformMatrix4fv(matricesLocation, MaxShadowMatrixUniforms, GL_TRUE, matrices.data());
        glUniform1f(uniform("uShadowBias"), prepared_.settings.bias);
        glUniform1f(uniform("uShadowSlopeBias"), prepared_.settings.slopeBias);
        glUniform1f(uniform("uShadowResolution"), static_cast<float>(resolution_));
        GLint previousActiveTexture = GL_TEXTURE0;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
        glActiveTexture(GL_TEXTURE0 + ShadowTextureUnit);
        glBindTexture(GL_TEXTURE_2D_ARRAY, depthArray_);
        glActiveTexture(static_cast<GLenum>(previousActiveTexture));
    }


    bool OpenGLShadowPass::active() const {
        return !prepared_.lights.empty() && depthArray_ != 0;
    }
}
