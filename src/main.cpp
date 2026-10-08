#include "core/Application.h"

#include "render/RenderBackend.h"



int main() {
    constexpr ve::RenderBackend backend =
        ve::RenderBackend::OpenGL;


    ve::Application application{
        backend
    };


    application.run();


    return 0;
}