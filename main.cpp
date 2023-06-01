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
  Print(PrintInfoType::Portracer, "Starting Portracer (_)=---=(_)");

  /// Initialize GLFW
  if (!glfwInit()) {
    Error(PrintInfoType::GLFW, "Could not initialize GLFW!");
    return 1;
  }
  /// Create Window
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow *window = glfwCreateWindow(640, 480, "Portracer (_)=---=(_)", NULL, NULL);

  /// Setup WebGPU
  WGPUInstanceDescriptor desc = {};
  desc.nextInChain = nullptr;

  /// Create WebGpu instance
  WGPUInstance instance = wgpuCreateInstance(&desc);
  if (!instance) {
    Error(PrintInfoType::WebGPU, "Could not initialize WebGPU!");
    return 1;
  }
  Print(PrintInfoType::WebGPU, "WebGPU instance: ", instance);

  /// Get WebGPU adapter
  Print(PrintInfoType::WebGPU, "Requesting adapter ...");
  WGPURequestAdapterOptions adapter_options = {};
  adapter_options.nextInChain = nullptr;
  WGPUSurface surface = glfwGetWGPUSurface(instance, window);
  adapter_options.compatibleSurface = surface;
  WGPUAdapter adapter = RequestAdapter(instance, &adapter_options);
  Print(PrintInfoType::WebGPU, "Got adapter:", adapter);

  /// Get WebGPU device
  Print(PrintInfoType::WebGPU, "Requesting device ...");
  // Minimal descriptor setting
  WGPUDeviceDescriptor device_desc = {};
  device_desc.nextInChain = nullptr;
  device_desc.label = "Portracer Device";
  device_desc.requiredFeaturesCount = 0;
  device_desc.requiredLimits = nullptr;
  device_desc.defaultQueue.nextInChain = nullptr;
  device_desc.defaultQueue.label = "Default Queue";
  WGPUDevice device = RequestDevice(adapter, &device_desc);
  Print(PrintInfoType::WebGPU, "Got device: ", device);
  // Error handling
  wgpuDeviceSetUncapturedErrorCallback(device, OnDeviceError, nullptr);

  /// Get device queue
  WGPUQueue queue = wgpuDeviceGetQueue(device);
  #ifdef WEBGPU_BACKEND_DAWN
  // signalValue is 0 for now
  wgpuQueueOnSubmittedWorkDone(queue, 0, OnQueueWorkDone, nullptr);
  #endif

  /// Create swap chain
  WGPUSwapChainDescriptor swap_chain_desc = {};
  swap_chain_desc.nextInChain = nullptr;
  swap_chain_desc.width = 640;
  swap_chain_desc.height = 480;
  /// Texture format
  #ifdef WEBGPU_BACKEND_WGPU
  WGPUTextureFormat swap_chain_format = wgpuSurfaceGetPreferredFormat(surface, adapter);
  swap_chain_desc.format = swap_chain_format;
  #else
  // For Dawn BGRA8Unorm only
  swap_chain_desc.format = WGPUTextureFormat_BGRA8Unorm;
  #endif
  swap_chain_desc.usage = WGPUTextureUsage_RenderAttachment;
  swap_chain_desc.presentMode = WGPUPresentMode_Fifo;
  WGPUSwapChain swap_chain = wgpuDeviceCreateSwapChain(device, surface, &swap_chain_desc);
  Print(PrintInfoType::WebGPU, "Swapchain: ", swap_chain);

  /// Use Window
  if (!window) {
    Error(PrintInfoType::GLFW, "Could not open window!");
    glfwTerminate();
    return 1;
  }
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Get target texture view
    WGPUTextureView next_texture = wgpuSwapChainGetCurrentTextureView(swap_chain);
    if (!next_texture) {
      Error(PrintInfoType::WebGPU, "Cannot acquire next swap chain texture");
      break;
    }
    Print(PrintInfoType::WebGPU, "next_texture: ", next_texture);

    // Draw
    /// Command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    encoder_desc.nextInChain = nullptr;
    encoder_desc.label = "Portracer Command Encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoder_desc);
    /// Create Render pass
    WGPURenderPassDescriptor render_pass_desc = {};
    WGPURenderPassColorAttachment render_pass_color_attachment = {};
    // Set the texture view directly
    render_pass_color_attachment.view = next_texture;
    // This is for multi-sampling
    render_pass_color_attachment.resolveTarget = nullptr;
    render_pass_color_attachment.loadOp = WGPULoadOp_Clear;
    render_pass_color_attachment.storeOp = WGPUStoreOp_Store;
    render_pass_color_attachment.clearValue = WGPUColor{0.274f, 0.886f, 0.745f, 1.0f};
    render_pass_desc.colorAttachmentCount = 1;
    render_pass_desc.colorAttachments = &render_pass_color_attachment;
    render_pass_desc.depthStencilAttachment = nullptr;
    render_pass_desc.timestampWriteCount = 0;
    render_pass_desc.timestampWrites = nullptr;
    render_pass_desc.nextInChain = nullptr;
    WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
    /// Just end the command for now
    wgpuRenderPassEncoderEnd(render_pass);

    // Destroy texture view
    wgpuTextureViewRelease(next_texture);

    /// Submit command queue
    WGPUCommandBufferDescriptor cmd_buffer_desc = {};
    cmd_buffer_desc.nextInChain = nullptr;
    cmd_buffer_desc.label = "Command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
    wgpuQueueSubmit(queue, 1, &command);

    // Present texture
    wgpuSwapChainPresent(swap_chain);
  }

  /// WebGPU stuff
  // Release WebGPU swap chain
  wgpuSwapChainRelease(swap_chain);
  // Release WebGPU device
  wgpuDeviceRelease(device);
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
