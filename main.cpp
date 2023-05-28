#include <iostream>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>
#include "utils/wgpu_util.h"
#include "utils/print_util.h"
#ifdef WEBGPU_BACKEND_WGPU
// wgpu-native's non-standard parts are in a different header file:
#include <webgpu/wgpu.h>
#define wgpuInstanceRelease wgpuInstanceDrop
#endif

int main() {
  std::cout << "Starting Portracer (_)=---=(_)" << std::endl;

  // Initialize GLFW
  if (!glfwInit()) {
    Error(PrintInfoType::GLFW, "Could not initialize GLFW!");
    return 1;
  }
  // Create Window
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow *window = glfwCreateWindow(640, 480, "Portracer (_)=---=(_)", NULL, NULL);

  // Setup WebGPU
  WGPUInstanceDescriptor desc = {};
  desc.nextInChain = nullptr;

  // Create WebGpu instance
  WGPUInstance instance = wgpuCreateInstance(&desc);
  if (!instance) {
    Error(PrintInfoType::WebGPU, "Could not initialize WebGPU!");
    return 1;
  }
  Print(PrintInfoType::WebGPU, "WebGPU instance: ", instance);

  // Get WebGPU adapter
  Print(PrintInfoType::WebGPU, "Requesting adapter ...");
  WGPURequestAdapterOptions adapter_options = {};
  adapter_options.nextInChain = nullptr;
  WGPUSurface surface = glfwGetWGPUSurface(instance, window);
  adapter_options.compatibleSurface = surface;
  WGPUAdapter adapter = RequestAdapter(instance, &adapter_options);
  Print(PrintInfoType::WebGPU, "Got adapter:", adapter);

  ShowAdapterFeature(adapter);

  // Use Window
  if (!window) {
    Error(PrintInfoType::GLFW, "Could not open window!");
    glfwTerminate();
    return 1;
  }
  while (!glfwWindowShouldClose(window)) {
    // Check whether the user clicked on the close button (and any other
    // mouse/key event, which we don't use so far)
    glfwPollEvents();
  }

  /// WebGPU stuff
  // Release WebGPU surface
  wgpuSurfaceRelease(surface);
  // Release WebGPU adapter
  wgpuAdapterRelease(adapter);
  // Release WebGPU instance
  wgpuInstanceRelease(instance);

  /// GLFW stuff
  // Destroy the Window
  glfwDestroyWindow(window);
  // Terminate GLFW
  glfwTerminate();
  return 0;
}
