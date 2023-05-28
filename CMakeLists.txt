cmake_minimum_required(VERSION 3.22)
project(
        Portracer
        VERSION 0.1.0
        LANGUAGES CXX C
)
set(CMAKE_CXX_STANDARD 17)

add_executable(Portracer main.cpp utils/wgpu_util.h utils/print_util.h)
add_subdirectory(glfw)
add_subdirectory(glfw3webgpu)
add_subdirectory(webgpu)
target_link_libraries(Portracer PRIVATE glfw webgpu glfw3webgpu)

set_target_properties(Portracer PROPERTIES
                      CXX_STANDARD 17
                      COMPILE_WARNING_AS_ERROR ON
                      )
# Warning Settings
if (MSVC)
    target_compile_options(Portracer PRIVATE /W4)
else ()
    target_compile_options(Portracer PRIVATE -Wall -Wextra -pedantic)
endif ()
# This might be unnecessary
target_copy_webgpu_binaries(Portracer)