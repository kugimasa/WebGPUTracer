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

  /// Get adapter capabilities
  SupportedLimits supported_limits;
  adapter.getLimits(&supported_limits);

  /// Get WebGPU device
  Print(PrintInfoType::WebGPU, "Requesting device ...");
  // Setting up required limits
  RequiredLimits requiredLimits = Default;
  requiredLimits.limits.maxBindGroups = 1;
  requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
  requiredLimits.limits.maxUniformBufferBindingSize = 16 * sizeof(float);
  // Without this, wgpu-native crashes
  requiredLimits.limits.maxBufferSize = 15 * 5 * sizeof(float);
  // This must be set even if we do not use storage buffers for now
  requiredLimits.limits.minStorageBufferOffsetAlignment = supported_limits.limits.minStorageBufferOffsetAlignment;
  // This must be set even if we do not use uniform buffers for now
  requiredLimits.limits.minUniformBufferOffsetAlignment = supported_limits.limits.minUniformBufferOffsetAlignment;
  // Minimal descriptor setting
  DeviceDescriptor device_desc = {};
  device_desc.label = "Portracer Device";
  device_desc.requiredFeaturesCount = 0;
  device_desc.requiredLimits = &requiredLimits;
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

  /// Shader source
  Print(PrintInfoType::WebGPU, "Creating shader module ...");
  ShaderModule shader_module = LoadShaderModule(RESOURCE_DIR "/shader/triangle.wgsl", device);
  Print(PrintInfoType::WebGPU, "Shader module: ", shader_module);

  Print(PrintInfoType::WebGPU, "Creating pipeline ...");
  /// Render pipeline setup
  RenderPipelineDescriptor pipeline_desc;
  /// Vertex pipeline state
  // Hard-code position for now
  pipeline_desc.vertex.bufferCount = 0;
  pipeline_desc.vertex.buffers = nullptr;
  pipeline_desc.vertex.module = shader_module;
  pipeline_desc.vertex.entryPoint = "vs_main";
  pipeline_desc.vertex.constantCount = 0;
  pipeline_desc.vertex.constants = nullptr;
  /// Primitive pipeline state
  pipeline_desc.primitive.topology = PrimitiveTopology::TriangleList;
  pipeline_desc.primitive.stripIndexFormat = IndexFormat::Undefined;
  pipeline_desc.primitive.frontFace = FrontFace::CCW;
  pipeline_desc.primitive.cullMode = CullMode::None;
  /// Fragment shader
  FragmentState fragment_state;
  fragment_state.module = shader_module;
  fragment_state.entryPoint = "fs_main";
  fragment_state.constantCount = 0;
  fragment_state.constants = nullptr;
  pipeline_desc.fragment = &fragment_state;
  /// Stencil/Depth
  pipeline_desc.depthStencil = nullptr;
  /// Blending
  BlendState blend_state;
  blend_state.color.srcFactor = BlendFactor::SrcAlpha;
  blend_state.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
  blend_state.color.operation = BlendOperation::Add;
  blend_state.alpha.srcFactor = BlendFactor::Zero;
  blend_state.alpha.dstFactor = BlendFactor::One;
  blend_state.alpha.operation = BlendOperation::Add;
  ColorTargetState color_target;
  #ifdef WEBGPU_BACKEND_WGPU
  color_target.format = swap_chain_format;
  #else
  // For Dawn BGRA8Unorm only
  color_target.format = TextureFormat::BGRA8Unorm;
  #endif
  color_target.blend = &blend_state;
  color_target.writeMask = ColorWriteMask::All;
  fragment_state.targetCount = 1;
  fragment_state.targets = &color_target;
  /// Multi-sampling
  pipeline_desc.multisample.count = 1;
  pipeline_desc.multisample.mask = ~0u;
  pipeline_desc.multisample.alphaToCoverageEnabled = false;
  /// Create binding layout
  BindGroupLayoutEntry binding_layout = Default;
  binding_layout.binding = 0;
  binding_layout.visibility = ShaderStage::Fragment;
  // We change here if we want to bind texture or samplers
  binding_layout.buffer.type = BufferBindingType::Uniform;
  binding_layout.buffer.minBindingSize = sizeof(float);
  /// Create a bind group layout
  BindGroupLayoutDescriptor bind_group_layout_desc{};
  bind_group_layout_desc.entryCount = 1;
  bind_group_layout_desc.entries = &binding_layout;
  BindGroupLayout bind_group_layout = device.createBindGroupLayout(bind_group_layout_desc);
  /// Create a pipeline layout
  PipelineLayoutDescriptor layout_desc{};
  layout_desc.bindGroupLayoutCount = 1;
  layout_desc.bindGroupLayouts = (WGPUBindGroupLayout *) &bind_group_layout;
  PipelineLayout layout = device.createPipelineLayout(layout_desc);
  pipeline_desc.layout = layout;
  /// Create a render pipeline
  RenderPipeline pipeline = device.createRenderPipeline(pipeline_desc);
  Print(PrintInfoType::WebGPU, "Render pipeline: ", pipeline);

  /// Create buffer
  BufferDescriptor buffer_desc{};
  /// Create uniform buffer
  // uTime & uIsWgpuNative
  // buffer_desc.size = 2 * sizeof(float);
  buffer_desc.size = sizeof(float);
  buffer_desc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
  buffer_desc.mappedAtCreation = false;
  Print(PrintInfoType::WebGPU, "----");
  Buffer uniform_buffer = device.createBuffer(buffer_desc);
  Print(PrintInfoType::WebGPU, "++++");
  float current_time = 1.0f;
  float is_wgpu_native = 0.0f;
  #ifdef WEBGPU_BACKEND_WGPU
  is_wgpu_native = 1.0f;
  #endif
  std::vector<float> uniform_data = {current_time, is_wgpu_native};
  queue.writeBuffer(uniform_buffer, 0, &current_time, buffer_desc.size);

  /// Create a binding
  BindGroupEntry binding{};
  binding.binding = 0;
  binding.buffer = uniform_buffer;
  // We set the offset to hold multiple uniform blocks
  binding.offset = 0;
  binding.size = sizeof(float);
  BindGroupDescriptor bind_group_desc{};
  bind_group_desc.layout = bind_group_layout;
  bind_group_desc.entryCount = bind_group_layout_desc.entryCount;
  bind_group_desc.entries = &binding;
  BindGroup bind_group = device.createBindGroup(bind_group_desc);

  /// Use Window
  if (!window) {
    Error(PrintInfoType::GLFW, "Could not open window!");
    glfwTerminate();
    return 1;
  }
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Update uniform buffer
    float t = static_cast<float>(glfwGetTime());
    queue.writeBuffer(uniform_buffer, 0, &t, sizeof(float));

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
    render_pass_color_attachment.loadOp = LoadOp::Clear;
    render_pass_color_attachment.storeOp = StoreOp::Store;
    render_pass_color_attachment.clearValue = WGPUColor{0.274f, 0.886f, 0.745f, 1.0f};
    render_pass_desc.colorAttachmentCount = 1;
    render_pass_desc.colorAttachments = &render_pass_color_attachment;
    render_pass_desc.depthStencilAttachment = nullptr;
    render_pass_desc.timestampWriteCount = 0;
    render_pass_desc.timestampWrites = nullptr;
    render_pass_desc.nextInChain = nullptr;
    RenderPassEncoder render_pass = encoder.beginRenderPass(render_pass_desc);
    /// Draw Call
    render_pass.setPipeline(pipeline);
    // Set binding group
    render_pass.setBindGroup(0, bind_group, 0, nullptr);
    render_pass.draw(3, 1, 0, 0);
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
