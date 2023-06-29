#pragma once
#include "GLFW/glfw3.h"
#include "glfw3webgpu.h"
#include "utils/wgpu_util.h"

class Renderer {
 public:
  bool OnInit();
  void OnCompute();
  void OnFrame();
  void OnFinish();
  bool IsRunning();

 private:
  GLFWwindow *window_ = nullptr;
  Instance instance_ = nullptr;
  Adapter adapter_ = nullptr;
  Device device_ = nullptr;
  Surface surface_ = nullptr;
  Queue queue_ = nullptr;
  SwapChain swap_chain_ = nullptr;
  Buffer uniform_buffer_ = nullptr;
  RenderPipeline pipeline_ = nullptr;
  BindGroup bind_group_ = nullptr;
};

/// \brief Initialize function
/// \return whether properly initialized
inline bool Renderer::OnInit() {
  /// Initialize GLFW
  if (!glfwInit()) {
    Error(PrintInfoType::GLFW, "Could not initialize GLFW!");
    return false;
  }
  /// Create Window
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window_ = glfwCreateWindow(640, 480, "Portracer (_)=---=(_)", NULL, NULL);

  /// Setup WebGPU
  InstanceDescriptor desc = {};
  /// Create WebGPU instance
  instance_ = createInstance(desc);
  if (!instance_) {
    Error(PrintInfoType::WebGPU, "Could not initialize WebGPU!");
    return false;
  }
  Print(PrintInfoType::WebGPU, "WebGPU instance: ", instance_);

  /// Get WebGPU adapter
  Print(PrintInfoType::WebGPU, "Requesting adapter ...");
  RequestAdapterOptions adapter_options = {};
  surface_ = glfwGetWGPUSurface(instance_, window_);
  adapter_options.compatibleSurface = surface_;
  adapter_ = instance_.requestAdapter(adapter_options);
  Print(PrintInfoType::WebGPU, "Got adapter:", adapter_);

  /// Get adapter capabilities
  SupportedLimits supported_limits;
  adapter_.getLimits(&supported_limits);

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
  device_ = adapter_.requestDevice(device_desc);
  Print(PrintInfoType::WebGPU, "Got device: ", device_);

  // Error handling
  device_.setUncapturedErrorCallback(OnDeviceError);

  /// Get device queue
  queue_ = device_.getQueue();
  #ifdef WEBGPU_BACKEND_DAWN
  // signalValue is 0 for now
  queue_.onSubmittedWorkDone(0, OnQueueWorkDone);
  #endif

  /// Create swap chain
  SwapChainDescriptor swap_chain_desc = {};
  swap_chain_desc.nextInChain = nullptr;
  swap_chain_desc.width = 640;
  swap_chain_desc.height = 480;
  /// Texture format
  #ifdef WEBGPU_BACKEND_WGPU
  TextureFormat swap_chain_format = surface_.getPreferredFormat(adapter_);
  swap_chain_desc.format = swap_chain_format;
  #else
  // For Dawn BGRA8Unorm only
  swap_chain_desc.format = TextureFormat::BGRA8Unorm;
  #endif
  swap_chain_desc.usage = TextureUsage::RenderAttachment;
  swap_chain_desc.presentMode = PresentMode::Fifo;
  swap_chain_ = device_.createSwapChain(surface_, swap_chain_desc);
  Print(PrintInfoType::WebGPU, "Swapchain: ", swap_chain_);

  /// Shader source
  Print(PrintInfoType::WebGPU, "Creating shader module ...");
  ShaderModule shader_module = LoadShaderModule(RESOURCE_DIR "/shader/triangle.wgsl", device_);
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
  BindGroupLayout bind_group_layout = device_.createBindGroupLayout(bind_group_layout_desc);
  /// Create a pipeline layout
  PipelineLayoutDescriptor layout_desc{};
  layout_desc.bindGroupLayoutCount = 1;
  layout_desc.bindGroupLayouts = (WGPUBindGroupLayout *) &bind_group_layout;
  PipelineLayout layout = device_.createPipelineLayout(layout_desc);
  pipeline_desc.layout = layout;
  /// Create a render pipeline
  pipeline_ = device_.createRenderPipeline(pipeline_desc);
  Print(PrintInfoType::WebGPU, "Render pipeline: ", pipeline_);

  /// Create buffer
  BufferDescriptor buffer_desc{};
  /// Create uniform buffer
  buffer_desc.size = sizeof(float);
  buffer_desc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
  buffer_desc.mappedAtCreation = false;
  uniform_buffer_ = device_.createBuffer(buffer_desc);
  float is_wgpu_native = 0.0f;
  #ifdef WEBGPU_BACKEND_WGPU
  is_wgpu_native = 1.0f;
  #endif
  queue_.writeBuffer(uniform_buffer_, 0, &is_wgpu_native, buffer_desc.size);

  /// Create a binding
  BindGroupEntry binding{};
  binding.binding = 0;
  binding.buffer = uniform_buffer_;
  // We set the offset to hold multiple uniform blocks
  binding.offset = 0;
  binding.size = sizeof(float);
  BindGroupDescriptor bind_group_desc{};
  bind_group_desc.layout = bind_group_layout;
  bind_group_desc.entryCount = bind_group_layout_desc.entryCount;
  bind_group_desc.entries = &binding;
  bind_group_ = device_.createBindGroup(bind_group_desc);
  return true;
}

inline void Renderer::OnCompute() {
  // Initialize a command encoder
  CommandEncoderDescriptor encoder_desc = Default;
  CommandEncoder encoder = device_.createCommandEncoder(encoder_desc);

  // Create and use compute pass
  // Encode and submit the GPU commands
  CommandBuffer commands = encoder.finish(CommandBufferDescriptor{});
  queue_.submit(commands);
  // Clean up
#ifdef WEBGPU_BACKEND_DAWN
  commands.release();
  encoder.release();
#endif
}

/// \brief Called every frame
inline void Renderer::OnFrame() {
  glfwPollEvents();

  // Get target texture view
  TextureView next_texture = swap_chain_.getCurrentTextureView();
  if (!next_texture) {
    Error(PrintInfoType::WebGPU, "Cannot acquire next swap chain texture");
    return;
  }
  Print(PrintInfoType::WebGPU, "next_texture: ", next_texture);

  // Draw
  /// Command encoder
  CommandEncoderDescriptor encoder_desc = {};
  CommandEncoder encoder = device_.createCommandEncoder(encoder_desc);
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
  render_pass.setPipeline(pipeline_);
  // Set binding group
  render_pass.setBindGroup(0, bind_group_, 0, nullptr);
  render_pass.draw(3, 1, 0, 0);
  /// Just end the command for now
  render_pass.end();

  // Destroy texture view
  next_texture.release();

  /// Submit command queue
  CommandBufferDescriptor cmd_buffer_desc = {};
  CommandBuffer command = encoder.finish(cmd_buffer_desc);
  queue_.submit(command);

  // Present texture
  swap_chain_.present();
}

/// \brief Called on application quit
inline void Renderer::OnFinish() {
  /// WebGPU stuff
  // Release WebGPU queue
  queue_.release();
  // Release WebGPU swap chain
  swap_chain_.release();
  // Release WebGPU device
  device_.release();
  // Release WebGPU surface
  surface_.release();
  // Release WebGPU adapter
  adapter_.release();
  // Release WebGPU instance
  instance_.release();

  /// GLFW stuff
  // Destroy the Window
  glfwDestroyWindow(window_);
  // Terminate GLFW
  glfwTerminate();
}

/// \brief Check if the renderer is running
/// \return Running or not
inline bool Renderer::IsRunning() {
  return !glfwWindowShouldClose(window_);
}
