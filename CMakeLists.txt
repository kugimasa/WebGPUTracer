cmake_minimum_required(VERSION 3.22)
project(
        Portracer
        VERSION 0.1.0
        LANGUAGES CXX C
)
set(CMAKE_CXX_STANDARD 17)

# Development option
option(DEV_MODE "Set up development helper settings" ON)

add_subdirectory(glfw)
add_subdirectory(glfw3webgpu)
add_subdirectory(webgpu)

add_executable(Portracer
               src/main.cpp
               src/render.cpp
               src/include/renderer.h
               src/utils/wgpu_util.h
               src/utils/print_util.h
               src/utils/save_texture.h
               external/implementation.cpp)

include_directories("external/stb")
include_directories("external/tinyobjloader")

# Output directory
target_compile_definitions(Portracer PRIVATE
                           OUTPUT_DIR="${CMAKE_BINARY_DIR}/output"
                           )
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/output)

if (DEV_MODE)
    target_compile_definitions(Portracer PRIVATE
                               RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
                               )
else ()
    file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})
    target_compile_definitions(Portracer PRIVATE
                               RESOURCE_DIR="./resources"
                               )
endif ()

target_link_libraries(Portracer PRIVATE glfw webgpu glfw3webgpu)

set_target_properties(Portracer PROPERTIES
                      CXX_STANDARD 17
                      COMPILE_WARNING_AS_ERROR ON
                      )
# Warning Settings
if (MSVC)
    set_target_properties(Portracer PROPERTIES VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1")
    target_compile_options(Portracer PRIVATE /W4)
else ()
    target_compile_options(Portracer PRIVATE -Wall -Wextra -pedantic)
endif ()
# This might be unnecessary
target_copy_webgpu_binaries(Portracer)