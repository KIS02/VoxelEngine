cmake_minimum_required(VERSION 3.20)

project(VoxelEngine
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# --------------------------------------------------
# GLFW
# --------------------------------------------------

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)


# --------------------------------------------------
# Executable
# --------------------------------------------------

add_executable(VoxelEngine

    src/main.cpp

    src/core/Application.cpp
    src/core/Window.cpp
)


target_include_directories(VoxelEngine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)


target_link_libraries(VoxelEngine
    PRIVATE
        glfw
)


# --------------------------------------------------
# Compiler warnings
# --------------------------------------------------

if(MSVC)

    target_compile_options(VoxelEngine PRIVATE
        /W4
    )

else()

    target_compile_options(VoxelEngine PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

endif()