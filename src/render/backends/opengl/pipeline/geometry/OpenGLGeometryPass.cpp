#include "render/backends/opengl/pipeline/geometry/OpenGLGeometryPass.h"

#include <cstdint>
#include <stdexcept>

#include "assets/material/MaterialData.h"
#include "assets/mesh/MeshData.h"
#include "assets/texture/Texture2D.h"
#include "render/backends/opengl/OpenGLRenderCommand.h"
#include "render/backends/opengl/OpenGLRenderer.h"
#include "render/backends/opengl/resources/OpenGLFrameResources.h"
#include "render/backends/opengl/resources/OpenGLResourceCache.h"
#include "render/frame/RenderFrame.h"
#include "render/lighting/Lighting.h"


namespace ve
{
    void OpenGLGeometryPass::draw( const RenderFrame& frame, OpenGLRenderer& renderer,
                                   OpenGLResourceCache& resources,
                                   BenchmarkRenderStatistics* statistics ) {
        OpenGLFrameResources active;

        for (const RenderDraw& draw : frame.draws) {
            if (draw.mesh == nullptr)
                throw std::invalid_argument("OpenGL Draw requires MeshData");

            if (draw.material == nullptr || draw.material->texture == nullptr)
                throw std::invalid_argument("OpenGL Draw requires a Material with Texture");

            if (draw.mesh->indexCount() == 0)
                continue;

            const MaterialData& material = *draw.material;
            validateMaterialLighting(material);

            const OpenGLMesh& gpuMesh = resources.resolveMesh(
                *draw.mesh, draw.meshId, draw.meshRevision, statistics
            );
            OpenGLMaterial& gpuMaterial = resources.resolveMaterial(*material.texture);

            OpenGLDrawCommand command{};
            command.mesh = &gpuMesh;
            command.material = &gpuMaterial;
            command.model = draw.model;
            command.directionalLight = frame.lighting.directional;
            command.specularStrength = material.specularStrength;
            command.shininess = material.shininess;
            command.diffuseTint = material.diffuseTint;
            command.specularColor = material.specularColor;
            command.lightingDebugMode = 0;

            active.track(draw.meshId, material.texture->id());
            renderer.submit(command, frame.view, frame.projection, frame.cameraPosition);
        }

        resources.releaseUnused(active.meshIds(), active.textureIds(), statistics);
    }
}
