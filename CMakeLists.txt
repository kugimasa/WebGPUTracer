cmake_minimum_required(VERSION 3.22)
project(
        WebGPUTracer
        VERSION 0.1.0
        LANGUAGES CXX C
)
set(CMAKE_CXX_STANDARD 17)

# Development option
option(DEV_MODE "Set up development helper settings" ON)

add_subdirectory(glfw)
add_subdirectory(glfw3webgpu)
add_subdirectory(webgpu)

add_executable(WebGPUTracer
               src/main.cpp
               src/camera.cpp
               src/render.cpp
               src/objects/box.cpp
               src/objects/cornell_box.cpp
               src/objects/triangle.cpp
               src/objects/quad.cpp
               src/objects/vertex.cpp
               src/scene.cpp
               external/implementation.cpp)

include_directories(src/include)
include_directories("external/stb")
include_directories("external/tinyobjloader")

# Output directory
target_compile_definitions(WebGPUTracer PRIVATE
                           OUTPUT_DIR="${CMAKE_BINARY_DIR}"
                           )
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/output)

if (DEV_MODE)
    target_compile_definitions(WebGPUTracer PRIVATE
                               RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
                               )
else ()
    file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})
    target_compile_definitions(WebGPUTracer PRIVATE
                               RESOURCE_DIR="./resources"
                               )
endif ()

target_link_libraries(WebGPUTracer PRIVATE glfw webgpu glfw3webgpu)

set_target_properties(WebGPUTracer PROPERTIES
                      CXX_STANDARD 17
                      )
# Warning Settings
if (MSVC)
    set_target_properties(WebGPUTracer PROPERTIES VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1")
    target_compile_options(WebGPUTracer PRIVATE /W4)
else ()
    target_compile_options(WebGPUTracer PRIVATE -Wall -Wextra -pedantic)
endif ()

if(XCODE)
    set_target_properties(WebGPUTracer PROPERTIES
                          XCODE_GENERATE_SCHEME ON
                          XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal")
endif()

# This might be unnecessary
target_copy_webgpu_binaries(WebGPUTracer)