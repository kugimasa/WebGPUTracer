#include "camera.h"
#include "objects/cornell_box.h"

/// \brief Constructor
/// \param device
Camera::Camera(Device &device) {
  // Create Bind Layout Group
  InitBindGroupLayout(device);
  // Create Buffer
  InitBuffers(device);
  // Create Bind Group
  InitBindGroup(device);
}

void Camera::Release() {
  uniforms_.bind_group_.release();
  uniform_buffer_.destroy();
  uniform_buffer_.release();
  uniforms_.bind_group_layout_.release();
}

void Camera::InitBindGroupLayout(Device &device) {
  std::vector<BindGroupLayoutEntry> bindings(2, Default);

  // Uniforms
  bindings[0].binding = 0;
  bindings[0].buffer.type = BufferBindingType::Uniform;
  bindings[0].visibility = ShaderStage::Compute;

  // Scene: Cornell Box
  bindings[1].binding = 1;
  bindings[1].buffer.type = BufferBindingType::ReadOnlyStorage;
  bindings[1].visibility = ShaderStage::Compute;

  /// Create a bind group layout
  BindGroupLayoutDescriptor bind_group_layout_desc{};
  bind_group_layout_desc.entryCount = (uint32_t) bindings.size();
  bind_group_layout_desc.entries = bindings.data();
  bind_group_layout_desc.label = "Camera.uniforms_.bind_group_layout_";
  uniforms_.bind_group_layout_ = device.createBindGroupLayout(bind_group_layout_desc);
}

void Camera::InitBuffers(Device &device) {
  buffer_size_ = sizeof(Ray);
  BufferDescriptor buffer_desc{};
  buffer_desc.mappedAtCreation = false;
  buffer_desc.size = buffer_size_;
  buffer_desc.usage = BufferUsage::Uniform | BufferUsage::CopyDst;
  buffer_desc.label = "Camera.uniform_buffer_";
  uniform_buffer_ = device.createBuffer(buffer_desc);
}

void Camera::InitBindGroup(Device &device) {
  /// Create a binding
  std::vector<BindGroupEntry> entries(2, Default);

  /// Uniform Buffer
  entries[0].binding = 0;
  entries[0].buffer = uniform_buffer_;
  entries[0].offset = 0;
  entries[0].size = buffer_size_;
  /// Output buffer
  CornellBox cb(device);
  entries[1].binding = 1;
  entries[1].buffer = cb.quad_buffer_;
  entries[1].offset = 0;
  entries[1].size = cb.quad_buffer_.getSize();
  BindGroupDescriptor bind_group_desc;
  bind_group_desc.layout = uniforms_.bind_group_layout_;
  bind_group_desc.entryCount = (uint32_t) entries.size();
  bind_group_desc.entries = (WGPUBindGroupEntry *) entries.data();
  uniforms_.bind_group_ = device.createBindGroup(bind_group_desc);
}

void Camera::Update(Queue &queue, Point3 origin, Point3 target, float aspect, float time) {
  Ray ray{origin, target, aspect, time, Rand()};
  queue.writeBuffer(uniform_buffer_, 0, &ray, sizeof(Ray));
}
