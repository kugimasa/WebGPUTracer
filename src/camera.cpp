#include "camera.h"

/// \brief Constructor
/// \param device
Camera::Camera(Device &device, uint32_t spp) {
  // Set sample per pixel
  spp_ = spp;
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
  std::vector<BindGroupLayoutEntry> bindings(1, Default);

  // Uniforms
  bindings[0].binding = 0;
  bindings[0].buffer.type = BufferBindingType::Uniform;
  bindings[0].visibility = ShaderStage::Compute;

  /// Create a bind group layout
  BindGroupLayoutDescriptor bind_group_layout_desc{};
  bind_group_layout_desc.entryCount = (uint32_t) bindings.size();
  bind_group_layout_desc.entries = bindings.data();
  bind_group_layout_desc.label = "Camera.uniforms_.bind_group_layout_";
  uniforms_.bind_group_layout_ = device.createBindGroupLayout(bind_group_layout_desc);
}

void Camera::InitBuffers(Device &device) {
  BufferDescriptor buffer_desc{};
  buffer_desc.mappedAtCreation = false;
  buffer_desc.size = sizeof(CameraParam);
  buffer_desc.usage = BufferUsage::Uniform | BufferUsage::CopyDst;
  buffer_desc.label = "Camera.uniform_buffer_";
  uniform_buffer_ = device.createBuffer(buffer_desc);
}

void Camera::InitBindGroup(Device &device) {
  /// Bindingを作成
  std::vector<BindGroupEntry> entries(1, Default);

  /// Uniform Buffer
  entries[0].binding = 0;
  entries[0].buffer = uniform_buffer_;
  entries[0].offset = 0;
  entries[0].size = sizeof(CameraParam);
  BindGroupDescriptor bind_group_desc;
  bind_group_desc.layout = uniforms_.bind_group_layout_;
  bind_group_desc.entryCount = (uint32_t) entries.size();
  bind_group_desc.entries = (WGPUBindGroupEntry *) entries.data();
  uniforms_.bind_group_ = device.createBindGroup(bind_group_desc);
}

void Camera::Update(Queue &queue, float t, float aspect) {
  Point3 origin = Vec3(278, 278, -800);
  Point3 target = Vec3(278, 278, 0);
  float fovy = 40.0f;
  CameraParam param(origin, target, aspect, fovy, spp_, RandSeed());
  queue.writeBuffer(uniform_buffer_, 0, &param, sizeof(CameraParam));
}
