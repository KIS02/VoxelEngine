
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
