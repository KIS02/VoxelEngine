#pragma once

#include "math/Mat4.h"



namespace ve
{
    class OpenGLMesh;
    class OpenGLMaterial;



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
    };
}