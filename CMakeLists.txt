cmake_minimum_required(VERSION 3.20)

project(VoxelEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Dependencies: keep GLFW and GLAD as build targets.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

add_library(glad STATIC external/glad/src/gl.c)
target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include")


# One compiled engine library is shared by the application and all test executables.
# CONFIGURE_DEPENDS re-runs CMake when a new source file is added or removed.
file(GLOB_RECURSE VE_ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
list(REMOVE_ITEM VE_ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

# Centralize include paths, C++ features, platform definitions, and warning flags.
add_library(ve_build_options INTERFACE)
target_include_directories(ve_build_options INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(ve_build_options INTERFACE GLFW_INCLUDE_NONE)
target_compile_features(ve_build_options INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(ve_build_options INTERFACE /W4)
else()
    target_compile_options(ve_build_options INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_library(VoxelEngineCore STATIC ${VE_ENGINE_SOURCES})
target_link_libraries(VoxelEngineCore PUBLIC ve_build_options glfw glad)

if(WIN32)
    target_link_libraries(VoxelEngineCore PUBLIC
        opengl32 gdi32 ole32 windowscodecs uuid
    )
endif()

add_executable(VoxelEngine src/main.cpp)
target_link_libraries(VoxelEngine PRIVATE VoxelEngineCore)


# Automatic unit/GPU test discovery: place each standalone main() in tests/*Tests.cpp.
# Test targets are excluded from the default ALL build; select only the ones needed.
include(CTest)

if(BUILD_TESTING)
    file(GLOB VE_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*Tests.cpp"
    )

    foreach(test_source IN LISTS VE_TEST_SOURCES)
        get_filename_component(test_name "${test_source}" NAME_WE)
        add_executable(${test_name} EXCLUDE_FROM_ALL "${test_source}")
        target_link_libraries(${test_name} PRIVATE VoxelEngineCore)

        # The CSV comparison requires two externally generated benchmark paths.
        if(test_name STREQUAL "BenchmarkCsvRegressionTests")
            continue()
        endif()

        add_test(NAME ${test_name} COMMAND ${test_name})
        set_tests_properties(${test_name} PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )

        if(test_name MATCHES "GpuParityTests$")
            set_tests_properties(${test_name} PROPERTIES LABELS "gpu")
        else()
            set_tests_properties(${test_name} PROPERTIES LABELS "unit")
        endif()
    endforeach()
endif()