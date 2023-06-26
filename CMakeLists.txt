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
               src/renderer.h
               src/utils/wgpu_util.h
               src/utils/print_util.h)

if (DEV_MODE)
    target_compile_definitions(Portracer PRIVATE
                               RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
                               )
else ()
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