#pragma once

#include "math/LinearRGB.h"
#include "math/Mat4.h"
#include "render/lighting/DirectionalLight.h"
#include "render/lighting/Lighting.h"


namespace ve
{
    class OpenGLMesh;
    class OpenGLMaterial;
    class OpenGLShadowPass;


    struct OpenGLClearCommand
    {
        float red = 0.0f;
        float green = 0.0f;
        float blue = 0.0f;
        float alpha = 1.0f;
        bool clearDepth = true;
    };


    struct OpenGLRenderStateCommand
    {
        bool depthTest = true;
        bool backFaceCulling = true;
    };


    struct OpenGLDrawCommand
    {
        const OpenGLMesh* mesh = nullptr;
        OpenGLMaterial* material = nullptr;

        Mat4 model = Mat4::identity();
        DirectionalLight directionalLight{};
        const LightingSettings* sceneLighting = nullptr;
        const OpenGLShadowPass* shadows = nullptr;
        bool hdrOutput = false;

        float specularStrength = 0.35f;
        float shininess = 32.0f;

        int lightingDebugMode = 0;
        LinearRGB diffuseTint{ 1.0f, 1.0f, 1.0f };
        LinearRGB specularColor{ 1.0f, 1.0f, 1.0f };
    };
}
