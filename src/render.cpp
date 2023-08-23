#include "renderer.h"
#include "camera.h"
#include "utils/save_texture.h"
#include "utils/util.h"

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
    window_ = glfwCreateWindow(WIDTH, HEIGHT, "Portracer (_)=---=(_)", NULL, NULL);
  }

  buffer_size_ = 64 * sizeof(float);   // seed
  if (!InitDevice()) return false;
  InitTexture();
  InitTextureViews();
  // InitSwapChain();
  InitBindGroupLayout();
  InitComputePipeline();
  // InitRenderPipeline();
  InitBuffers();
  InitBindGroup();
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
  requiredLimits.limits.maxBindGroups = 3;
  requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
  requiredLimits.limits.maxUniformBufferBindingSize = 16 * sizeof(float);
  // origin(3), target(3), aspect(1), time(1), seed(1)
  // Without this, wgpu-native crashes
  requiredLimits.limits.maxVertexAttributes = 3;
  requiredLimits.limits.maxVertexBufferArrayStride = 10 * sizeof(float);
  requiredLimits.limits.maxVertexBuffers = 3;
  requiredLimits.limits.maxBufferSize = 100000 * sizeof(Vertex);
  requiredLimits.limits.maxTextureDimension1D = 4096;
  requiredLimits.limits.maxTextureDimension2D = 4096;
  // Cannot be 4096 on local macOS (wgpu-native)
  requiredLimits.limits.maxTextureDimension3D = 2048;
  requiredLimits.limits.maxTextureArrayLayers = 1;
  requiredLimits.limits.maxStorageBuffersPerShaderStage = 3;
  requiredLimits.limits.maxStorageBufferBindingSize = 100000 * sizeof(Vertex);
  requiredLimits.limits.maxStorageTexturesPerShaderStage = 1;
  requiredLimits.limits.maxComputeWorkgroupSizeX = 80;
  requiredLimits.limits.maxComputeWorkgroupSizeY = 60;
  requiredLimits.limits.maxComputeWorkgroupSizeZ = 1;
  requiredLimits.limits.maxComputeInvocationsPerWorkgroup = 64;
  requiredLimits.limits.maxComputeWorkgroupsPerDimension = 80;
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

  /// カメラの初期化
  camera_ = Camera(device_);
  /// シーンの初期化
  scene_ = Scene(device_);

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
  swap_chain_desc.width = 640;
  swap_chain_desc.height = 480;
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

/// \brief WebGPU BindGroupLayout
void Renderer::InitBindGroupLayout() {
  Print(PrintInfoType::WebGPU, "Create bind group layout ...");
  std::vector<BindGroupLayoutEntry> bindings(2, Default);

  // Input buffer
  bindings[0].binding = 0;
  bindings[0].buffer.type = BufferBindingType::ReadOnlyStorage;
  bindings[0].visibility = ShaderStage::Compute;

  // Output buffer
  bindings[1].binding = 1;
  bindings[1].storageTexture.access = StorageTextureAccess::WriteOnly;
  bindings[1].storageTexture.format = TextureFormat::RGBA8Unorm;
  bindings[1].storageTexture.viewDimension = TextureViewDimension::_2D;
  bindings[1].visibility = ShaderStage::Compute;

  /// Create a bind group layout
  BindGroupLayoutDescriptor bind_group_layout_desc{};
  bind_group_layout_desc.entryCount = (uint32_t) bindings.size();
  bind_group_layout_desc.entries = bindings.data();
  bind_group_layout_ = device_.createBindGroupLayout(bind_group_layout_desc);
}

/// \brief WebGPU RenderPipeline setup
void Renderer::InitRenderPipeline() {
  Print(PrintInfoType::WebGPU, "Creating render pipeline ...");
  /// Shader source
  Print(PrintInfoType::WebGPU, "Creating shader module ...");
  ShaderModule shader_module = LoadShaderModule(RESOURCE_DIR "/shader/triangle.wgsl", device_);
  Print(PrintInfoType::WebGPU, "Shader module: ", shader_module);
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
  color_target.format = swap_chain_format_;
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
  ShaderModule shader_module = LoadShaderModule(RESOURCE_DIR "/shader/compute-sample.wgsl", device_);
  Print(PrintInfoType::WebGPU, "Shader module: ", shader_module);

  /// Create a pipeline layout
  PipelineLayoutDescriptor layout_desc{};
  std::vector<WGPUBindGroupLayout> bind_group_layouts{camera_.GetUniforms().bind_group_layout_,
                                                      scene_.storage_.bind_group_layout_,
                                                      bind_group_layout_};
  layout_desc.bindGroupLayoutCount = 3;
  layout_desc.bindGroupLayouts = (WGPUBindGroupLayout *) bind_group_layouts.data();
  pipeline_layout_ = device_.createPipelineLayout(layout_desc);

  /// Compute pipeline setup
  ComputePipelineDescriptor pipeline_desc;
  pipeline_desc.compute.constantCount = 0;
  pipeline_desc.compute.constants = nullptr;
  pipeline_desc.compute.entryPoint = "compute_sample";
  pipeline_desc.compute.module = shader_module;
  pipeline_desc.layout = pipeline_layout_;
  /// Create a compute pipeline
  compute_pipeline_ = device_.createComputePipeline(pipeline_desc);
  Print(PrintInfoType::WebGPU, "Compute pipeline: ", compute_pipeline_);
}

/// \brief WebGPU Buffer setup
void Renderer::InitBuffers() {
  Print(PrintInfoType::WebGPU, "Creating buffers ...");
  BufferDescriptor buffer_desc{};
  buffer_desc.mappedAtCreation = false;
  /// Create input buffer
  buffer_desc.size = buffer_size_;
  buffer_desc.usage = BufferUsage::Storage | BufferUsage::CopyDst;
  input_buffer_ = device_.createBuffer(buffer_desc);
  Print(PrintInfoType::WebGPU, "Input buffer: ", input_buffer_);
  /// Create output buffer
  buffer_desc.usage = BufferUsage::Storage | BufferUsage::CopySrc;
  output_buffer_ = device_.createBuffer(buffer_desc);
  Print(PrintInfoType::WebGPU, "Output buffer: ", output_buffer_);
}

/// \brief WebGPU BindGroup setup
void Renderer::InitBindGroup() {
  Print(PrintInfoType::WebGPU, "Creating bind group ...");
  /// Create a binding
  std::vector<BindGroupEntry> entries(2, Default);

  /// Input buffer
  entries[0].binding = 0;
  entries[0].buffer = input_buffer_;
  entries[0].offset = 0;
  entries[0].size = buffer_size_;
  /// Output buffer
  entries[1].binding = 1;
  entries[1].textureView = output_texture_view_;

  BindGroupDescriptor bind_group_desc;
  bind_group_desc.layout = bind_group_layout_;
  bind_group_desc.entryCount = (uint32_t) entries.size();
  bind_group_desc.entries = (WGPUBindGroupEntry *) entries.data();
  bind_group_ = device_.createBindGroup(bind_group_desc);
  Print(PrintInfoType::WebGPU, "Bind group: ", bind_group_);
}

/// \brief Compute pass
bool Renderer::OnCompute() {
  Print(PrintInfoType::Portracer, "Running compute pass ...");
  auto success = false;
  // chrono変数
  std::chrono::system_clock::time_point start, end;
  // 時間計測開始
  start = std::chrono::system_clock::now();
  for (uint32_t i = 0; i < MAX_FRAME; ++i) {
    success = OnRender(i);
  }
  queue_.release();
  // 時間計測終了
  end = std::chrono::system_clock::now();
  // 経過時間の算出
  double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  std::ostringstream sout;
  sout << elapsed * 0.001 << "(sec)s";
  Print(PrintInfoType::Portracer, "Finished: ", sout.str());
  return success;
}

bool Renderer::OnRender(int frame) {
  // chrono変数
  std::chrono::system_clock::time_point start, end;
  // 時間計測開始
  start = std::chrono::system_clock::now();
  /// Update camera
  /// NOTE: 原点が(0, 0, 0)だと描画がうまくいかないことがある(FarのQuadなど)
  Point3 origin = Vec3(0, 0, 0.01);
  Point3 target = Vec3(0, 0, 15);
  float aspect = (float) WIDTH / (float) HEIGHT;
  float time = 0.0f;
  camera_.Update(queue_, origin, target, aspect, time);

  /// Input buffer
  std::vector<float> input(buffer_size_ / sizeof(float));
  for (int i = 0; i < (int) input.size(); ++i) {
    input[i] = 0.1f * (float) i;
  }
  queue_.writeBuffer(input_buffer_, 0, input.data(), input.size() * sizeof(float));

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
  compute_pass.setBindGroup(1, scene_.storage_.bind_group_, 0, nullptr);
  compute_pass.setBindGroup(2, bind_group_, 0, nullptr);

  uint32_t invocation_count_x = texture_size_.width;
  uint32_t invocation_count_y = texture_size_.height;
  uint32_t workgroup_size_per_dim = 8;
  // This ceils invocationCountX / workgroupSizePerDim
  uint32_t workgroup_count_x = (invocation_count_x + workgroup_size_per_dim - 1) / workgroup_size_per_dim;
  uint32_t workgroup_count_y = (invocation_count_y + workgroup_size_per_dim - 1) / workgroup_size_per_dim;
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
  std::string output_file = OUTPUT_DIR "/" + sout.str() + ".png";
  if (!saveTexture(output_file.c_str(), device_, texture_, pixel_buffer_, 0 /* output MIP level */)) {
    Error(PrintInfoType::Portracer, "Image output failed.");
    return false;
  }
  // Clean up
  commands.release();
  encoder.release();
  compute_pass.release();
  // 時間計測終了
  end = std::chrono::system_clock::now();
  // 経過時間の算出
  double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  std::cout << "[" << sout.str() << "]: " << elapsed * 0.001 << "(sec)s" << std::endl;
  return true;
}

/// \brief Called every frame
void Renderer::OnFrame() {
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
  render_pass.setPipeline(render_pipeline_);
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
void Renderer::OnFinish() {
  /// Release Camera
  camera_.Release();
  /// Release Scene
  scene_.Release();
  /// WebGPU stuff
  /// Release WebGPU bind group
  bind_group_.release();
  /// Release WebGPU texture views
  output_texture_view_.release();
  /// Release WebGPU texture
  texture_.destroy();
  texture_.release();
  /// Release WebGPU buffer
  // uniform_buffer_.destroy();
  // uniform_buffer_.release();
  input_buffer_.destroy();
  input_buffer_.release();
  // output_buffer_.destroy();
  // output_buffer_.release();
  // map_buffer_.destroy();
  // map_buffer_.release();
  pixel_buffer_.destroy();
  pixel_buffer_.release();
  /// Release WebGPU pipelines
  // render_pipeline_.release();
  compute_pipeline_.release();
  pipeline_layout_.release();
  /// Release WebGPU bind group layout
  bind_group_layout_.release();
  /// Release WebGPU swap chain
  // swap_chain_.release();
  /// Release WebGPU device
  device_.release();
  /// Release WebGPU surface
  // surface_.release();
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
