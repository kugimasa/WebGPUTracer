#include "include/camera.h"

/// \brief Constructor
/// \param device
Camera::Camera(Device device) {
  device_ = device;
  // Create Bind Layout Group
  InitBindGroupLayout();
  // Create Buffer
  InitBuffers();
  // Create Bind Group
  InitBindGroup();
}

/// \brief Destructor
Camera::~Camera() {
  uniforms_.bind_group_.release();
  uniform_buffer_.destroy();
  uniform_buffer_.release();
  uniforms_.bind_group_layout_.release();
}

void Camera::InitBindGroupLayout() {
  std::vector<BindGroupLayoutEntry> bindings(1, Default);

  // Uniforms
  bindings[0].binding = 0;
  bindings[0].buffer.type = BufferBindingType::Uniform;
  bindings[0].visibility = ShaderStage::Compute;

  // TODO: Scene data
//  bindings[1].binding = 1;
//  bindings[1].buffer.type = BufferBindingType::ReadOnlyStorage;
//  bindings[1].visibility = ShaderStage::Compute;

  /// Create a bind group layout
  BindGroupLayoutDescriptor bind_group_layout_desc{};
  bind_group_layout_desc.entryCount = (uint32_t) bindings.size();
  bind_group_layout_desc.entries = bindings.data();
  bind_group_layout_desc.label = "Camera.uniforms_.bind_group_layout_";
  uniforms_.bind_group_layout_ = device_.createBindGroupLayout(bind_group_layout_desc);
}

void Camera::InitBuffers() {
  buffer_size_ = 0 +
                 16 * sizeof(float) + // mvp
                 16 * sizeof(float) + // inv_mvp
                 4 * sizeof(float);   // seed
  BufferDescriptor buffer_desc{};
  buffer_desc.mappedAtCreation = false;
  buffer_desc.size = buffer_size_;
  buffer_desc.usage = BufferUsage::Uniform | BufferUsage::CopyDst;
  buffer_desc.label = "Camera.uniform_buffer_";
  uniform_buffer_ = device_.createBuffer(buffer_desc);
}

void Camera::InitBindGroup() {
  /// Create a binding
  std::vector<BindGroupEntry> entries(1, Default);

  /// Uniform Buffer
  entries[0].binding = 0;
  entries[0].buffer = uniform_buffer_;
  entries[0].offset = 0;
  entries[0].size = buffer_size_;
  /// Output buffer
//  entries[1].binding = 1;
//  entries[1].buffer = uniform_buffer_;
//  entries[1].offset = 0;
//  entries[1].size = buffer_size_;
  BindGroupDescriptor bind_group_desc;
  bind_group_desc.layout = uniforms_.bind_group_layout_;
  bind_group_desc.entryCount = (uint32_t) entries.size();
  bind_group_desc.entries = (WGPUBindGroupEntry *) entries.data();
  uniforms_.bind_group_ = device_.createBindGroup(bind_group_desc);
}