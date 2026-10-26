
cmake_minimum_required(VERSION 3.20)

project(VoxelEngine
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# --------------------------------------------------
# GLFW
# --------------------------------------------------

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)


# --------------------------------------------------
# GLAD
# --------------------------------------------------

add_library(glad STATIC
    external/glad/src/gl.c
)

target_include_directories(glad
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
)


# --------------------------------------------------
# Executable
# --------------------------------------------------

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
)

add_executable(VoxelEngine ${SOURCES})

target_include_directories(VoxelEngine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(VoxelEngine
    PRIVATE
        GLFW_INCLUDE_NONE
)


# --------------------------------------------------
# Libraries
# --------------------------------------------------

target_link_libraries(VoxelEngine
    PRIVATE
        glfw
        glad
)

if(WIN32)
    target_link_libraries(VoxelEngine
        PRIVATE
            opengl32
            gdi32
    )
endif()


# --------------------------------------------------
# Compiler warnings
# --------------------------------------------------

if(MSVC)
    target_compile_options(VoxelEngine
        PRIVATE
            /W4
    )
else()
    target_compile_options(VoxelEngine
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()


# --------------------------------------------------
#  GPU Lighting Parity Test
# --------------------------------------------------

add_executable(LightingGpuParityTests
    tests/LightingGpuParityTests.cpp

    src/assets/mesh/MeshData.cpp

    src/render/backends/opengl/OpenGLRenderer.cpp
    src/render/backends/opengl/pipeline/lighting/OpenGLLightingBindings.cpp
    src/render/backends/opengl/OpenGLMesh.cpp
    src/render/backends/opengl/OpenGLMaterial.cpp
    src/render/backends/opengl/OpenGLTexture2D.cpp
    src/render/backends/opengl/Shader.cpp

    src/render/color/ColorSpace.cpp
    src/render/lighting/Lighting.cpp
    src/render/lighting/DirectionalLight.cpp
    src/render/lighting/NormalMatrix3.cpp
)

target_include_directories(LightingGpuParityTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(LightingGpuParityTests PRIVATE
    GLFW_INCLUDE_NONE
)

target_link_libraries(LightingGpuParityTests PRIVATE
    glfw
    glad
)

if(WIN32)
    target_link_libraries(LightingGpuParityTests PRIVATE
        opengl32
        gdi32
    )
endif()

# --------------------------------------------------
# RenderFrame GPU Parity Tests
# --------------------------------------------------

if(NOT TARGET LightingGpuParityTests)
    message(FATAL_ERROR
        "LightingGpuParityTests target is required"
    )
endif()

get_target_property(
    RENDER_FRAME_PARITY_SOURCES
    LightingGpuParityTests
    SOURCES
)

list(
    FILTER RENDER_FRAME_PARITY_SOURCES
    EXCLUDE REGEX "LightingGpuParityTests[.]cpp$"
)

list(APPEND RENDER_FRAME_PARITY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/RenderFrameGpuParityTests.cpp"
)

foreach(source_name IN ITEMS
    FrameBuffer.cpp
    DepthBuffer.cpp
    Rasterizer.cpp
    Texture2D.cpp
    RasterInterpolation.cpp
)
    file(GLOB_RECURSE source_paths CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/${source_name}"
    )

    list(LENGTH source_paths source_count)

    if(NOT source_count EQUAL 1)
        message(FATAL_ERROR
            "Expected exactly one ${source_name}, found ${source_count}: ${source_paths}"
        )
    endif()

    list(APPEND RENDER_FRAME_PARITY_SOURCES
        "${source_paths}"
    )
endforeach()

list(APPEND RENDER_FRAME_PARITY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/render/backends/software/pipeline/shading/SoftwareFragmentShading.cpp"
)

add_executable(
    RenderFrameGpuParityTests
    ${RENDER_FRAME_PARITY_SOURCES}
)

foreach(property IN ITEMS
    INCLUDE_DIRECTORIES
    COMPILE_DEFINITIONS
    COMPILE_OPTIONS
    COMPILE_FEATURES
    LINK_LIBRARIES
    LINK_OPTIONS
    CXX_STANDARD
    CXX_STANDARD_REQUIRED
    CXX_EXTENSIONS
)
    get_target_property(value LightingGpuParityTests ${property})

    if(NOT "${value}" STREQUAL "value-NOTFOUND")
        set_property(
            TARGET RenderFrameGpuParityTests
            PROPERTY ${property} "${value}"
        )
    endif()
endforeach()

# --------------------------------------------------
# Colored Lighting CPU/GPU Tests
# --------------------------------------------------

add_executable(ColoredLightingTests
    tests/ColoredLightingTests.cpp
    src/render/color/ColorSpace.cpp
    src/render/lighting/Lighting.cpp
)

target_include_directories(ColoredLightingTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(MSVC)
    target_compile_options(ColoredLightingTests PRIVATE /W4)
else()
    target_compile_options(ColoredLightingTests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(ColoredLightingGpuParityTests
    tests/ColoredLightingGpuParityTests.cpp
    src/assets/mesh/MeshData.cpp
    src/render/color/ColorSpace.cpp
    src/render/lighting/Lighting.cpp
    src/render/lighting/NormalMatrix3.cpp
    src/render/backends/opengl/OpenGLRenderer.cpp
    src/render/backends/opengl/pipeline/lighting/OpenGLLightingBindings.cpp
    src/render/backends/opengl/OpenGLMesh.cpp
    src/render/backends/opengl/OpenGLMaterial.cpp
    src/render/backends/opengl/OpenGLTexture2D.cpp
    src/render/backends/opengl/Shader.cpp
)

target_include_directories(ColoredLightingGpuParityTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(ColoredLightingGpuParityTests PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(ColoredLightingGpuParityTests PRIVATE glfw glad)

if(WIN32)
    target_link_libraries(ColoredLightingGpuParityTests PRIVATE opengl32 gdi32)
endif()

if(MSVC)
    target_compile_options(ColoredLightingGpuParityTests PRIVATE /W4)
else()
    target_compile_options(ColoredLightingGpuParityTests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# IMPORTANT: For existing test targets that directly compile Lighting.cpp,
# ALSO add src/render/color/ColorSpace.cpp to their sources; it is now required.

add_executable(ColoredShaderContractTests tests/ColoredShaderContractTests.cpp)
target_include_directories(ColoredShaderContractTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# --------------------------------------------------
# Phase 11.0 Software Pipeline Regression
# --------------------------------------------------

add_executable(SoftwareGeometryPipelineTests
    tests/SoftwareGeometryPipelineTests.cpp
    src/assets/mesh/MeshData.cpp
    src/assets/texture/Texture2D.cpp
    src/render/color/ColorSpace.cpp
    src/render/lighting/Lighting.cpp
    src/render/lighting/NormalMatrix3.cpp
    src/render/backends/software/FrameBuffer.cpp
    src/render/backends/software/DepthBuffer.cpp
    src/render/backends/software/Rasterizer.cpp
    src/render/backends/software/RasterInterpolation.cpp
    src/render/backends/software/pipeline/shading/SoftwareFragmentShading.cpp
    src/render/backends/software/pipeline/geometry/SoftwareGeometryPass.cpp
)

target_include_directories(SoftwareGeometryPipelineTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(SoftwareGeometryPipelineTests PRIVATE /W4)
else()
    target_compile_options(SoftwareGeometryPipelineTests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(OpenGLFrameResourcesTests
    tests/OpenGLFrameResourcesTests.cpp
)

target_include_directories(OpenGLFrameResourcesTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(OpenGLFrameResourcesTests PRIVATE /W4)
else()
    target_compile_options(OpenGLFrameResourcesTests PRIVATE -Wall -Wextra -Wpedantic)
endif()
