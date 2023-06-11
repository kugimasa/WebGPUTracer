#include "GLFW/glfw3.h"
#include "glfw3webgpu.h"
#include "utils/wgpu_util.h"
#include "utils/print_util.h"

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
  InstanceDescriptor desc = {};
  /// Create WebGpu instance
  Instance instance = createInstance(desc);
  if (!instance) {
    Error(PrintInfoType::WebGPU, "Could not initialize WebGPU!");
    return 1;
  }
  Print(PrintInfoType::WebGPU, "WebGPU instance: ", instance);

  /// Get WebGPU adapter
  Print(PrintInfoType::WebGPU, "Requesting adapter ...");
  RequestAdapterOptions adapter_options = {};
  Surface surface = glfwGetWGPUSurface(instance, window);
  adapter_options.compatibleSurface = surface;
  Adapter adapter = instance.requestAdapter(adapter_options);
  Print(PrintInfoType::WebGPU, "Got adapter:", adapter);

  /// Get WebGPU device
  Print(PrintInfoType::WebGPU, "Requesting device ...");
  // Minimal descriptor setting
  DeviceDescriptor device_desc = {};
  device_desc.nextInChain = nullptr;
  device_desc.label = "Portracer Device";
  device_desc.requiredFeaturesCount = 0;
  device_desc.requiredLimits = nullptr;
  device_desc.defaultQueue.nextInChain = nullptr;
  device_desc.defaultQueue.label = "Default Queue";
  Device device = adapter.requestDevice(device_desc);
  Print(PrintInfoType::WebGPU, "Got device: ", device);
  // Error handling
  device.setUncapturedErrorCallback(OnDeviceError);

  /// Get device queue
  Queue queue = device.getQueue();
  #ifdef WEBGPU_BACKEND_DAWN
  // signalValue is 0 for now
  queue.onSubmittedWorkDone(0, OnQueueWorkDone);
  #endif

  /// Create swap chain
  SwapChainDescriptor swap_chain_desc = {};
  swap_chain_desc.nextInChain = nullptr;
  swap_chain_desc.width = 640;
  swap_chain_desc.height = 480;
  /// Texture format
  #ifdef WEBGPU_BACKEND_WGPU
  TextureFormat swap_chain_format = surface.getPreferredFormat(adapter);
  swap_chain_desc.format = swap_chain_format;
  #else
  // For Dawn BGRA8Unorm only
  swap_chain_desc.format = TextureFormat::BGRA8Unorm;
  #endif
  swap_chain_desc.usage = TextureUsage::RenderAttachment;
  swap_chain_desc.presentMode = PresentMode::Fifo;
  SwapChain swap_chain = device.createSwapChain(surface, swap_chain_desc);
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
    TextureView next_texture = swap_chain.getCurrentTextureView();
    if (!next_texture) {
      Error(PrintInfoType::WebGPU, "Cannot acquire next swap chain texture");
      break;
    }
    Print(PrintInfoType::WebGPU, "next_texture: ", next_texture);

    // Draw
    /// Command encoder
    CommandEncoderDescriptor encoder_desc = {};
    CommandEncoder encoder = device.createCommandEncoder(encoder_desc);
    /// Create Render pass
    RenderPassDescriptor render_pass_desc = {};
    RenderPassColorAttachment render_pass_color_attachment = {};
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
    RenderPassEncoder render_pass = encoder.beginRenderPass(render_pass_desc);
    /// Just end the command for now
    render_pass.end();

    // Destroy texture view
    next_texture.release();

    /// Submit command queue
    CommandBufferDescriptor cmd_buffer_desc = {};
    CommandBuffer command = encoder.finish(cmd_buffer_desc);
    queue.submit(command);

    // Present texture
    swap_chain.present();
  }

  /// WebGPU stuff
  // Release WebGPU swap chain
  swap_chain.release();
  // Release WebGPU device
  device.release();
  // Release WebGPU surface
  surface.release();
  // Release WebGPU adapter
  adapter.release();
  // Release WebGPU instance
  instance.release();

  /// GLFW stuff
  // Destroy the Window
  glfwDestroyWindow(window);
  // Terminate GLFW
  glfwTerminate();
  return 0;
}
