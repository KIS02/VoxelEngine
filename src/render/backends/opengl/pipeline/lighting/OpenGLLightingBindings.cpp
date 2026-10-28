#include "render/backends/opengl/pipeline/lighting/OpenGLLightingBindings.h"

#include <stdexcept>
#include <array>
#include <cmath>
#include <string>

#include "render/backends/opengl/OpenGLMaterial.h"
#include "render/backends/opengl/OpenGLRenderCommand.h"
#include "render/backends/opengl/pipeline/shadow/OpenGLShadowPass.h"
#include "render/lighting/NormalMatrix3.h"

#include <glad/gl.h>


namespace ve
{
    void OpenGLLightingBindings::apply( const OpenGLDrawCommand& command, const Mat4& view,
                                        const Mat4& projection, const Vec3& cameraPosition ) {
        const GLuint programId = command.material->programId();

        const auto requireUniform = [programId]( const char* name ) {
            const GLint location = glGetUniformLocation(programId, name);

            if (location == -1)
                throw std::runtime_error(std::string("Missing Shader Uniform: ") + name);

            return location;
        };

        const GLint modelLocation = requireUniform("uModel");
        const GLint viewLocation = requireUniform("uView");
        const GLint projectionLocation = requireUniform("uProjection");
        const GLint normalMatrixLocation = requireUniform("uNormalMatrix");
        const GLint lightDirectionLocation = requireUniform("uDirectionToLight");
        const GLint ambientLocation = requireUniform("uAmbientStrength");
        const GLint diffuseLocation = requireUniform("uDiffuseStrength");
        const GLint lightColorLocation = requireUniform("uLightColor");
        const GLint cameraLocation = requireUniform("uCameraPosition");
        const GLint specularLocation = requireUniform("uSpecularStrength");
        const GLint shininessLocation = requireUniform("uShininess");
        const GLint diffuseTintLocation = requireUniform("uDiffuseTint");
        const GLint specularColorLocation = requireUniform("uSpecularColor");
        const GLint debugLocation = requireUniform("uLightingDebugMode");

        const NormalMatrix3 normalMatrix = calculateNormalMatrix(command.model);
        const DirectionalLight& light = command.directionalLight;
        const Vec3 direction = light.directionToLight.normalized();

        glUniformMatrix4fv(modelLocation, 1, GL_TRUE, &command.model.m[0][0]);
        glUniformMatrix4fv(viewLocation, 1, GL_TRUE, &view.m[0][0]);
        glUniformMatrix4fv(projectionLocation, 1, GL_TRUE, &projection.m[0][0]);
        glUniformMatrix3fv(normalMatrixLocation, 1, GL_TRUE, &normalMatrix.m[0][0]);

        glUniform3f(lightDirectionLocation, direction.x, direction.y, direction.z);
        glUniform1f(ambientLocation, light.ambient);
        glUniform1f(diffuseLocation, light.diffuse);
        glUniform3f(lightColorLocation, light.color.r, light.color.g, light.color.b);
        glUniform3f(cameraLocation, cameraPosition.x, cameraPosition.y, cameraPosition.z);
        glUniform1f(specularLocation, command.specularStrength);
        glUniform1f(shininessLocation, command.shininess);
        glUniform3f(diffuseTintLocation, command.diffuseTint.r, command.diffuseTint.g, command.diffuseTint.b);
        glUniform3f(specularColorLocation, command.specularColor.r,
                    command.specularColor.g, command.specularColor.b);
        glUniform1i(debugLocation, command.lightingDebugMode);

        const bool useSceneLights = command.sceneLighting != nullptr &&
                                    (!command.sceneLighting->sceneLights.empty() ||
                                     (command.directionalLight.ambient == 0.0f &&
                                      command.directionalLight.diffuse == 0.0f));
        glUniform1i(requireUniform("uUseSceneLights"), useSceneLights ? 1 : 0);
        glUniform1i(requireUniform("uSceneLightCount"),
                    useSceneLights ? static_cast<GLint>(command.sceneLighting->sceneLights.size()) : 0);

        // The Shadow Pass owns texture array storage, not Scene or FrameBuilder.
        if (command.shadows != nullptr) {
            command.shadows->bindForLighting(programId);
        } else {
            glUniform1i(requireUniform("uUseShadows"), 0);
        }

        if (useSceneLights) {
            std::array<GLint, MaxSceneLights> types{};
            std::array<GLfloat, MaxSceneLights * 4> positionRange{};
            std::array<GLfloat, MaxSceneLights * 4> emissionInner{};
            std::array<GLfloat, MaxSceneLights * 4> colorIntensity{};
            std::array<GLfloat, MaxSceneLights * 4> attenuation{};
            constexpr float Radians = 3.14159265358979323846f / 180.0f;

            for (std::size_t i = 0; i < command.sceneLighting->sceneLights.size(); ++i) {
                const SceneLightData& source = command.sceneLighting->sceneLights[i];
                const std::size_t offset = i * 4;
                types[i] = static_cast<GLint>(source.type);
                positionRange[offset] = source.position.x;
                positionRange[offset + 1] = source.position.y;
                positionRange[offset + 2] = source.position.z;
                positionRange[offset + 3] = source.range;

                const Vec3 direction = source.emissionDirection.normalized();
                emissionInner[offset] = direction.x;
                emissionInner[offset + 1] = direction.y;
                emissionInner[offset + 2] = direction.z;
                emissionInner[offset + 3] = std::cos(source.innerConeDegrees * Radians);

                colorIntensity[offset] = source.color.r;
                colorIntensity[offset + 1] = source.color.g;
                colorIntensity[offset + 2] = source.color.b;
                colorIntensity[offset + 3] = source.intensity;

                attenuation[offset] = source.ambient;
                attenuation[offset + 1] = source.diffuse;
                attenuation[offset + 2] = std::cos(source.outerConeDegrees * Radians);
            }

            const GLsizei count = static_cast<GLsizei>(command.sceneLighting->sceneLights.size());
            glUniform1iv(requireUniform("uLightTypes[0]"), count, types.data());
            glUniform4fv(requireUniform("uLightPositionRange[0]"), count, positionRange.data());
            glUniform4fv(requireUniform("uLightEmissionInnerCos[0]"), count, emissionInner.data());
            glUniform4fv(requireUniform("uLightColorIntensity[0]"), count, colorIntensity.data());
            glUniform4fv(requireUniform("uLightAttenuation[0]"), count, attenuation.data());
        }

    }
}
