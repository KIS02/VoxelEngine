#pragma once


namespace ve
{
    struct OpenGLDrawCommand;
    struct Mat4;
    struct Vec3;


    class OpenGLLightingBindings {
        public:
            static void apply( const OpenGLDrawCommand& command, const Mat4& view,
                               const Mat4& projection, const Vec3& cameraPosition );
    };
}
