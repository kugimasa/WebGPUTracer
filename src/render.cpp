#include "renderer.h"
#include "camera.h"
#include "utils/save_texture.h"
#include "utils/util.h"
#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

/// \brief Initialize function
/// \param hasWindow Uses window by glfw if true
/// \return whether properly initialized
bool Renderer::OnInit(bool hasWindow) {
  hasWindow_ = hasWindow;
  if (hasWindow_) {
    /// Initialize GLFW
    if (!glfwInit()) {
      Error(PrintInfoType::GLFW, "Could not initialize GLFW!");
      return false;
    }
    /// Create Window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(WIDTH, HEIGHT, "WebGPUTracer (_)=---=(_)", NULL, NULL);
    if (!window_) {
      Error(PrintInfoType::GLFW, "Could not open window!");
      return false;
    }
  }

  if (!InitDevice()) return false;
  // InitTexture();
  // InitTextureViews();
  if (hasWindow_) {
    InitSwapChain();
    InitRenderPipeline();
    InitDepthBuffer();
    InitDepthTextureView();
  }
  // InitComputePipeline();
  InitBuffers();
  InitBindGroup();
  /// TODO: Gui
  // if (!InitGui()) return false;
  return true;
}

/// \brief WebGPU Device setup
/// \return
bool Renderer::InitDevice() {
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
  if (hasWindow_) {
    surface_ = glfwGetWGPUSurface(instance_, window_);
    adapter_options.compatibleSurface = surface_;
    Print(PrintInfoType::WebGPU, "Got surface:", surface_);
  } else {
    adapter_options.compatibleSurface = nullptr;
  }
  adapter_ = instance_.requestAdapter(adapter_options);
  Print(PrintInfoType::WebGPU, "Got adapter:", adapter_);

  /// Get adapter capabilities
  SupportedLimits supported_limits;
  adapter_.getLimits(&supported_limits);

  /// Get WebGPU device
  Print(PrintInfoType::WebGPU, "Requesting device ...");
  // Setting up required limits
  RequiredLimits requiredLimits = Default;
  // Without this, wgpu-native crashes
  requiredLimits.limits.maxVertexAttributes = 2;
  requiredLimits.limits.maxVertexBuffers = 1;
  requiredLimits.limits.maxBufferSize = 15 * 5 * sizeof(float);
  requiredLimits.limits.maxVertexBufferArrayStride = 6 * sizeof(float);
  // This must be set even if we do not use storage buffers for now
  requiredLimits.limits.minStorageBufferOffsetAlignment = supported_limits.limits.minStorageBufferOffsetAlignment;
  // This must be set even if we do not use uniform buffers for now
  requiredLimits.limits.minUniformBufferOffsetAlignment = supported_limits.limits.minUniformBufferOffsetAlignment;
  // Number of components transiting from vertex to fragment shader
  requiredLimits.limits.maxInterStageShaderComponents = 3;
  requiredLimits.limits.maxBindGroups = 1;
  requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
  requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4;
  // For the depth buffer, we enable texture
  requiredLimits.limits.maxTextureDimension1D = WIDTH;
  requiredLimits.limits.maxTextureDimension2D = HEIGHT;
  // Cannot be 4096 on local macOS (wgpu-native)
  requiredLimits.limits.maxTextureDimension3D = 2048;
  requiredLimits.limits.maxTextureArrayLayers = 1;
  requiredLimits.limits.maxStorageBuffersPerShaderStage = 3;
  requiredLimits.limits.maxStorageBufferBindingSize = WIDTH * HEIGHT * sizeof(float);;
  requiredLimits.limits.maxStorageTexturesPerShaderStage = 1;
  // For Compute Pipeline
  // requiredLimits.limits.maxComputeWorkgroupSizeX = 32;
  // requiredLimits.limits.maxComputeWorkgroupSizeY = 32;
  // requiredLimits.limits.maxComputeWorkgroupSizeZ = 1;
  // requiredLimits.limits.maxComputeInvocationsPerWorkgroup = 256;
  // requiredLimits.limits.maxComputeWorkgroupsPerDimension = 32;
  // Minimal descriptor setting
  DeviceDescriptor device_desc = {};
  device_desc.label = "WebGPUTracer Device";
  device_desc.requiredFeaturesCount = 0;
  device_desc.requiredLimits = &requiredLimits;
  device_desc.defaultQueue.label = "Default Queue";
  device_ = adapter_.requestDevice(device_desc);
  Print(PrintInfoType::WebGPU, "Got device: ", device_);

  // Error handling
  auto onDeviceError = [](WGPUErrorType type, char const *message, void * /* pUserData */) {
      std::cout << "Uncaptured device error: type " << type;
      if (message) std::cout << " (" << message << ")";
      std::cout << std::endl;
  };
  wgpuDeviceSetUncapturedErrorCallback(device_, onDeviceError, nullptr /* pUserData */);

#ifdef WEBGPU_BACKEND_DAWN
  // Device lost callback
  wgpuDeviceSetDeviceLostCallback(device_, [](WGPUDeviceLostReason reason, char const *message, void *) {
      Print(PrintInfoType::WebGPU, "Device lost! Reason: ", reason);
      Print(PrintInfoType::WebGPU, "Device lost! message: ", message);
  }, nullptr);
#endif

  // TODO: Leave this for Compute Pipeline
  /// Initialize Camera
  // camera_ = Camera(device_, SPP);
  /// Initialize Scene
  // scene_ = Scene(device_);

  /// Get device queue
  queue_ = device_.getQueue();
#ifdef WEBGPU_BACKEND_DAWN
  instance_.processEvents();
#endif
  return true;
}

/// \brief Texture setup
void Renderer::InitTexture() {
  Print(PrintInfoType::WebGPU, "Creating texture ...");
  TextureDescriptor textureDesc;
  textureDesc.dimension = TextureDimension::_2D;
  textureDesc.format = TextureFormat::RGBA8Unorm;
  textureDesc.size = texture_size_;
  textureDesc.sampleCount = 1;
  textureDesc.viewFormatCount = 0;
  textureDesc.viewFormats = nullptr;
  textureDesc.usage = TextureUsage::StorageBinding | // Writing texture in shader
                      TextureUsage::CopySrc;         // Saving output data
  textureDesc.mipLevelCount = 1;
  texture_ = device_.createTexture(textureDesc);
  Print(PrintInfoType::WebGPU, "Got texture: ", texture_);
}

/// \brief WebGPU input/output Texture View setup
void Renderer::InitTextureViews() {
  Print(PrintInfoType::WebGPU, "Creating texture view ...");
  TextureViewDescriptor texture_view_desc;
  texture_view_desc.aspect = TextureAspect::All;
  texture_view_desc.baseArrayLayer = 0;
  texture_view_desc.arrayLayerCount = 1;
  texture_view_desc.dimension = TextureViewDimension::_2D;
  texture_view_desc.format = TextureFormat::RGBA8Unorm;
  texture_view_desc.mipLevelCount = 1;
  texture_view_desc.baseMipLevel = 0;
  texture_view_desc.label = "Output View";
  output_texture_view_ = texture_.createView(texture_view_desc);
  Print(PrintInfoType::WebGPU, "Got texture view: ", output_texture_view_);
}

/// \brief WebGPU SwapChain setup
void Renderer::InitSwapChain() {
  SwapChainDescriptor swap_chain_desc = {};
  swap_chain_desc.nextInChain = nullptr;
  swap_chain_desc.width = WIDTH;
  swap_chain_desc.height = HEIGHT;
  /// Texture format
#ifdef WEBGPU_BACKEND_WGPU
  swap_chain_format_ = surface_.getPreferredFormat(adapter_);
  swap_chain_desc.format = swap_chain_format_;
#else
  // For Dawn BGRA8Unorm only
  swap_chain_desc.format = TextureFormat::BGRA8Unorm;
#endif
  swap_chain_desc.usage = TextureUsage::RenderAttachment;
  swap_chain_desc.presentMode = PresentMode::Fifo;
  swap_chain_ = device_.createSwapChain(surface_, swap_chain_desc);
  Print(PrintInfoType::WebGPU, "Swapchain: ", swap_chain_);
}

/// \brief WebGPU Depth Buffer setup
void Renderer::InitDepthBuffer() {
  Print(PrintInfoType::WebGPU, "Creating depth texture ...");
  // Create depth texture
  TextureDescriptor depth_texture_desc;
  depth_texture_desc.dimension = TextureDimension::_2D;
  depth_texture_desc.format = depth_texture_format_;
  depth_texture_desc.mipLevelCount = 1;
  depth_texture_desc.sampleCount = 1;
  depth_texture_desc.size = texture_size_;
  depth_texture_desc.usage = TextureUsage::RenderAttachment;
  depth_texture_desc.viewFormatCount = 1;
  depth_texture_desc.viewFormats = (WGPUTextureFormat *) &depth_texture_format_;
  depth_texture_ = device_.createTexture(depth_texture_desc);
  Print(PrintInfoType::WebGPU, "Depth texture: ", depth_texture_);
}

/// \brief WebGPU Depth Texture View setup
void Renderer::InitDepthTextureView() {
  // Create the view of the depth texture manipulated by the rasterizer
  Print(PrintInfoType::WebGPU, "Creating depth texture view");
  TextureViewDescriptor depth_texture_view_desc;
  depth_texture_view_desc.aspect = TextureAspect::DepthOnly;
  depth_texture_view_desc.baseArrayLayer = 0;
  depth_texture_view_desc.arrayLayerCount = 1;
  depth_texture_view_desc.baseMipLevel = 0;
  depth_texture_view_desc.mipLevelCount = 1;
  depth_texture_view_desc.dimension = TextureViewDimension::_2D;
  depth_texture_view_desc.format = depth_texture_format_;
  depth_texture_view_ = depth_texture_.createView(depth_texture_view_desc);
  Print(PrintInfoType::WebGPU, "Depth texture view: ", depth_texture_view_);
}

/// \brief WebGPU BindGroupLayout
void Renderer::InitBindGroupLayout() {
  Print(PrintInfoType::WebGPU, "Create bind group layout ...");
  BindGroupLayoutEntry binding_layout = Default;

  // Input buffer
  binding_layout.binding = 0;
  binding_layout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
  binding_layout.buffer.type = BufferBindingType::Uniform;
  binding_layout.buffer.minBindingSize = sizeof(RenderParam);

  /// Create a bind group layout
  BindGroupLayoutDescriptor bind_group_layout_desc{};
  bind_group_layout_desc.entryCount = 1;
  bind_group_layout_desc.entries = &binding_layout;
  bind_group_layout_ = device_.createBindGroupLayout(bind_group_layout_desc);
  Print(PrintInfoType::WebGPU, "BindGroupLayout: ", bind_group_layout_);
}

/// \brief WebGPU RenderPipeline setup
void Renderer::InitRenderPipeline() {
  Print(PrintInfoType::WebGPU, "Creating render pipeline ...");
  /// Shader source
  Print(PrintInfoType::WebGPU, "Creating shader module ...");
  ShaderModule shader_module = LoadShaderModule(RESOURCE_DIR "/shader/shader.wgsl", device_);
  Print(PrintInfoType::WebGPU, "Shader module: ", shader_module);
  /// Render pipeline setup
  RenderPipelineDescriptor pipeline_desc;

  // Vertex fetch
  std::vector<VertexAttribute> vertex_attribs(2);
  // Position
  vertex_attribs[0].shaderLocation = 0;
  vertex_attribs[0].format = VertexFormat::Float32x3;
  vertex_attribs[0].offset = 0;
  // Color
  vertex_attribs[1].shaderLocation = 1;
  vertex_attribs[1].format = VertexFormat::Float32x3;
  vertex_attribs[1].offset = 3 * sizeof(float);

  VertexBufferLayout vertex_buffer_layout;
  vertex_buffer_layout.attributeCount = (uint32_t) vertex_attribs.size();
  vertex_buffer_layout.attributes = vertex_attribs.data();
  vertex_buffer_layout.arrayStride = 6 * sizeof(float);
  vertex_buffer_layout.stepMode = VertexStepMode::Vertex;

  /// Vertex pipeline state
  pipeline_desc.vertex.bufferCount = 1;
  pipeline_desc.vertex.buffers = &vertex_buffer_layout;
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
  color_target.format = swap_chain_format_;
#else
  // For Dawn BGRA8Unorm only
  color_target.format = TextureFormat::BGRA8Unorm;
#endif
  color_target.blend = &blend_state;
  color_target.writeMask = ColorWriteMask::All;
  fragment_state.targetCount = 1;
  fragment_state.targets = &color_target;
  /// Stencil/Depth
  DepthStencilState depth_stencil_state = Default;
  depth_stencil_state.depthCompare = CompareFunction::Less;
  depth_stencil_state.depthWriteEnabled = true;
  depth_stencil_state.format = depth_texture_format_;
  depth_stencil_state.stencilReadMask = 0;
  depth_stencil_state.stencilWriteMask = 0;
  pipeline_desc.depthStencil = &depth_stencil_state;
  /// Multi-sampling
  pipeline_desc.multisample.count = 1;
  pipeline_desc.multisample.mask = ~0u;
  pipeline_desc.multisample.alphaToCoverageEnabled = false;
  /// Create bind group layout
  InitBindGroupLayout();
  /// Create a pipeline layout
  PipelineLayoutDescriptor layout_desc{};
  layout_desc.bindGroupLayoutCount = 1;
  layout_desc.bindGroupLayouts = (WGPUBindGroupLayout *) &bind_group_layout_;
  pipeline_layout_ = device_.createPipelineLayout(layout_desc);
  pipeline_desc.layout = pipeline_layout_;
  /// Create a render pipeline
  render_pipeline_ = device_.createRenderPipeline(pipeline_desc);
  Print(PrintInfoType::WebGPU, "Render pipeline: ", render_pipeline_);
}

/// \brief WebGPU ComputePipeline setup
void Renderer::InitComputePipeline() {
  Print(PrintInfoType::WebGPU, "Creating compute pipeline ...");
  /// Shader source
  Print(PrintInfoType::WebGPU, "Creating shader module ...");
  ShaderModule shader_module = LoadShaderModule(RESOURCE_DIR "/shader/path_tracer.wgsl", device_);
  Print(PrintInfoType::WebGPU, "Shader module: ", shader_module);

  /// Create a pipeline layout
  PipelineLayoutDescriptor layout_desc{};
  std::vector<WGPUBindGroupLayout> bind_group_layouts{camera_.GetUniforms().bind_group_layout_,
                                                      scene_.objects_.bind_group_layout_,
                                                      bind_group_layout_};
  layout_desc.bindGroupLayoutCount = 3;
  layout_desc.bindGroupLayouts = (WGPUBindGroupLayout *) bind_group_layouts.data();
  Print(PrintInfoType::WebGPU, "Creating pipeline layout ...");
  pipeline_layout_ = device_.createPipelineLayout(layout_desc);
  Print(PrintInfoType::WebGPU, "Compute pipeline: ", pipeline_layout_);

  /// Compute pipeline setup
  ComputePipelineDescriptor pipeline_desc;
  pipeline_desc.compute.constantCount = 0;
  pipeline_desc.compute.constants = nullptr;
  pipeline_desc.compute.entryPoint = "compute_sample";
  pipeline_desc.compute.module = shader_module;
  pipeline_desc.layout = pipeline_layout_;
  /// Create a compute pipeline
  Print(PrintInfoType::WebGPU, "Creating compute pipeline ...");
  compute_pipeline_ = device_.createComputePipeline(pipeline_desc);
  Print(PrintInfoType::WebGPU, "Compute pipeline: ", compute_pipeline_);
}

/// \brief WebGPU Buffer setup
void Renderer::InitBuffers() {
  /// Load geometry
  std::vector<float> point_data;
  std::vector<uint16_t> index_data;
  if (!LoadGeometry(RESOURCE_DIR "/geometry/txt/pyramid.txt", point_data, index_data, 3)) {
    Error(PrintInfoType::WebGPUTracer, "Could not load geometry!");
  }
  vertex_buffer_size_ = point_data.size();
  index_buffer_size_ = index_data.size();
  uniform_buffer_size_ = sizeof(RenderParam);
  Print(PrintInfoType::WebGPU, "Creating buffers ...");
  BufferDescriptor buffer_desc{};
  buffer_desc.mappedAtCreation = false;
  /// Create vertex buffer
  buffer_desc.size = vertex_buffer_size_ * sizeof(float);
  buffer_desc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
  vertex_buffer_ = device_.createBuffer(buffer_desc);
  Print(PrintInfoType::WebGPU, "Vertex buffer: ", vertex_buffer_);
  queue_.writeBuffer(vertex_buffer_, 0, point_data.data(), buffer_desc.size);
  /// Create index buffer
  buffer_desc.size = index_buffer_size_ * sizeof(float);
  buffer_desc.usage = BufferUsage::CopyDst | BufferUsage::Index;
  index_buffer_ = device_.createBuffer(buffer_desc);
  Print(PrintInfoType::WebGPU, "Index buffer: ", index_buffer_);
  queue_.writeBuffer(index_buffer_, 0, index_data.data(), buffer_desc.size);
  /// Create uniform buffer
  buffer_desc.size = uniform_buffer_size_;
  buffer_desc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
  uniform_buffer_ = device_.createBuffer(buffer_desc);
  render_param_ = RenderParam(Color3(0.0f, 1.0f, 0.4f), 1.0f);
  queue_.writeBuffer(uniform_buffer_, 0, &render_param_, buffer_desc.size);
}

/// \brief WebGPU BindGroup setup
void Renderer::InitBindGroup() {
  Print(PrintInfoType::WebGPU, "Creating bind group ...");
  /// Create a binding
  BindGroupEntry binding{};
  /// Uniform buffer
  binding.binding = 0;
  binding.buffer = uniform_buffer_;
  binding.offset = 0;
  binding.size = uniform_buffer_size_;

  BindGroupDescriptor bind_group_desc;
  bind_group_desc.layout = bind_group_layout_;
  bind_group_desc.entryCount = 1;
  bind_group_desc.entries = &binding;
  bind_group_ = device_.createBindGroup(bind_group_desc);
  Print(PrintInfoType::WebGPU, "Bind group: ", bind_group_);
}

/// \brief Compute pass
bool Renderer::OnCompute(uint32_t start_frame, uint32_t end_frame) {
  Print(PrintInfoType::WebGPUTracer, "Running compute pass ...");
  auto success = false;
  // chrono変数
  std::chrono::system_clock::time_point start, end;
  // 時間計測開始
  start = std::chrono::system_clock::now();
  for (uint32_t i = start_frame - 1; i < end_frame; ++i) {
    success = OnRender(i);
  }
  queue_.release();
  // 時間計測終了
  end = std::chrono::system_clock::now();
  // 経過時間の算出
  double elapsed = (double) std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  std::ostringstream sout;
  sout << elapsed * 0.001 << "(sec)s";
  Print(PrintInfoType::WebGPUTracer, "Finished: ", sout.str());
  return success;
}

bool Renderer::OnRender(uint32_t frame) {
  // chrono変数
  std::chrono::system_clock::time_point start, end;
  // 時間計測開始
  start = std::chrono::system_clock::now();
  float t = (float) frame / (float) MAX_FRAME;
  /// Update camera
  float aspect = (float) WIDTH / (float) HEIGHT;
  camera_.Update(queue_, t, aspect);

  // Initialize a command encoder
  CommandEncoderDescriptor encoder_desc = Default;
  CommandEncoder encoder = device_.createCommandEncoder(encoder_desc);

  // Create compute pass
  ComputePassDescriptor compute_pass_desc;
  compute_pass_desc.timestampWriteCount = 0;
  compute_pass_desc.timestampWrites = nullptr;
  ComputePassEncoder compute_pass = encoder.beginComputePass(compute_pass_desc);

  // Use compute pass
  compute_pass.setPipeline(compute_pipeline_);
  compute_pass.setBindGroup(0, camera_.GetUniforms().bind_group_, 0, nullptr);
  compute_pass.setBindGroup(1, scene_.objects_.bind_group_, 0, nullptr);
  compute_pass.setBindGroup(2, bind_group_, 0, nullptr);

  uint32_t invocation_count_x = texture_size_.width;
  uint32_t invocation_count_y = texture_size_.height;
  uint32_t workgroup_size_x = 16;
  uint32_t workgroup_size_y = 16;
  // This ceils invocationCountX / workgroupSizePerDim
  uint32_t workgroup_count_x = (invocation_count_x + workgroup_size_x - 1) / workgroup_size_x;
  uint32_t workgroup_count_y = (invocation_count_y + workgroup_size_y - 1) / workgroup_size_y;
  compute_pass.dispatchWorkgroups(workgroup_count_x, workgroup_count_y, 1);

  // Finalize compute pass
  compute_pass.end();

  // Encode and submit the GPU commands
  CommandBuffer commands = encoder.finish(CommandBufferDescriptor{});
  queue_.submit(commands);
  // Save image
  /// PNG出力
  std::ostringstream sout;
  sout << std::setw(3) << std::setfill('0') << frame;
  std::string output_file = sout.str() + ".png";
  if (!saveTexture(output_file.c_str(), device_, texture_, 0 /* output MIP level */)) {
    Error(PrintInfoType::WebGPUTracer, "Image output failed.");
    return false;
  }
  // Clean up
  commands.release();
  encoder.release();
  compute_pass.release();
  // 時間計測終了
  end = std::chrono::system_clock::now();
  // 経過時間の算出
  double elapsed = (double) std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  std::cout << "[" << sout.str() << "]: " << elapsed * 0.001 << "(sec)s" << std::endl;
  return true;
}

/// \brief Called every frame
void Renderer::OnFrame() {
  glfwPollEvents();

  // Update uniform buffer
  render_param_.time = static_cast<float>(glfwGetTime());
  queue_.writeBuffer(uniform_buffer_, offsetof(RenderParam, time), &render_param_.time, sizeof(RenderParam::time));

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
  encoder_desc.label = "Command Encoder";
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
  render_pass_color_attachment.clearValue = WGPUColor{0.05, 0.05, 0.05, 1.0};
  render_pass_desc.colorAttachmentCount = 1;
  render_pass_desc.colorAttachments = &render_pass_color_attachment;
  // Depth buffer
  RenderPassDepthStencilAttachment depth_stencil_attachment;
  depth_stencil_attachment.view = depth_texture_view_;
  depth_stencil_attachment.depthClearValue = 1.0f;
  depth_stencil_attachment.depthLoadOp = LoadOp::Clear;
  depth_stencil_attachment.depthStoreOp = StoreOp::Store;
  depth_stencil_attachment.depthReadOnly = false;
  depth_stencil_attachment.stencilClearValue = 0;
#ifdef WEBGPU_BACKEND_WGPU
  depth_stencil_attachment.stencilLoadOp = LoadOp::Clear;
  depth_stencil_attachment.stencilStoreOp = StoreOp::Store;
#else
  depth_stencil_attachment.stencilLoadOp = LoadOp::Undefined;
  depth_stencil_attachment.stencilStoreOp = StoreOp::Undefined;
#endif
  depth_stencil_attachment.stencilReadOnly = true;
  render_pass_desc.depthStencilAttachment = &depth_stencil_attachment;
  render_pass_desc.timestampWriteCount = 0;
  render_pass_desc.timestampWrites = nullptr;
  render_pass_desc.nextInChain = nullptr;
  RenderPassEncoder render_pass = encoder.beginRenderPass(render_pass_desc);
  /// Draw Call
  render_pass.setPipeline(render_pipeline_);

  render_pass.setVertexBuffer(0, vertex_buffer_, 0, vertex_buffer_size_ * sizeof(float));
  render_pass.setIndexBuffer(index_buffer_, IndexFormat::Uint16, 0, index_buffer_size_ * sizeof(uint16_t));

  // Set binding group
  render_pass.setBindGroup(0, bind_group_, 0, nullptr);
  render_pass.drawIndexed(static_cast<int>(index_buffer_size_), 1, 0, 0, 0);

  /// TODO: Gui
  // UpdateGui(render_pass);

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
#ifdef WEBGPU_BACKEND_DAWN
  device_.tick();
#endif
}

/// \brief Called on application quit
void Renderer::OnFinish() {
  /// TODO: Dear ImGui
  // TerminateGui();
  /// TODO: Leave this for compute pipeline
  /// Release Camera
  // camera_.Release();
  /// Release Scene
  // scene_.Release();
  /// WebGPU stuff
  /// Release WebGPU bind group
  bind_group_.release();
  /// Release WebGPU buffer
  uniform_buffer_.destroy();
  uniform_buffer_.release();
  index_buffer_.destroy();
  index_buffer_.release();
  vertex_buffer_.destroy();
  vertex_buffer_.release();
  /// Release WebGPU pipelines
  render_pipeline_.release();
  // compute_pipeline_.release();
  pipeline_layout_.release();
  /// Release WebGPU bind group layout
  bind_group_layout_.release();
  /// Release WebGPU depth buffer
  depth_texture_view_.release();
  depth_texture_.destroy();
  depth_texture_.release();
  /// Release WebGPU swap chain
  swap_chain_.release();
  /// Release WebGPU texture views
  // output_texture_view_.release();
  /// Release WebGPU texture
  // texture_.destroy();
  // texture_.release();
  /// Release WebGPU device
  device_.release();
  /// Release WebGPU surface
  surface_.release();
  /// Release WebGPU adapter
  adapter_.release();
  /// Release WebGPU instance
  instance_.release();

  /// GLFW stuff
  if (hasWindow_) {
    // Destroy the Window
    glfwDestroyWindow(window_);
    // Terminate GLFW
    glfwTerminate();
  }
}

/// \brief Check if the renderer is running
/// \return Running or not
bool Renderer::IsRunning() {
  return !glfwWindowShouldClose(window_);
}

/// \brief Setup Dear ImGui
/// \return
bool Renderer::InitGui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOther(window_, true);
  ImGui_ImplWGPU_Init(device_, 3, swap_chain_format_, depth_texture_format_);

  return true;
}

void Renderer::TerminateGui() {
  ImGui_ImplGlfw_Shutdown();
  ImGui_ImplWGPU_Shutdown();
}

void Renderer::UpdateGui(RenderPassEncoder render_pass) {
  // TODO: Just a sample
  // Start Dear ImGui frame
  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // Build UI
  // Build our UI
  static float f = 0.0f;
  static int counter = 0;
  static bool show_demo_window = true;
  static bool show_another_window = false;
  static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  ImGui::Begin("Hello, world!");

  ImGui::Text("This is some useful text.");
  ImGui::Checkbox("Demo Window", &show_demo_window);
  ImGui::Checkbox("Another Window", &show_another_window);

  ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
  ImGui::ColorEdit3("clear color", (float *) &clear_color);

  if (ImGui::Button("Button"))
    counter++;
  ImGui::SameLine();
  ImGui::Text("counter = %d", counter);

  ImGuiIO &io = ImGui::GetIO();
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
  ImGui::End();

  // Draw UI
  ImGui::EndFrame();
  ImGui::Render();
  ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), render_pass);
}
