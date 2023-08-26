#include "camera.h"

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
  // カメラ: [前半]InQuart, [後半]等速
  float end_dist = 8.0f * EaseInQuart((0.95f - 0.2f) / 0.8f + 1.0f);
  float move_dist = t < 0.2f ? 8.0f * EaseInQuart(t / 0.2f) : t < 0.95f ? 8.0f * EaseInQuart((t - 0.2f) / 0.8f + 1.0f) : end_dist;
  Point3 origin = Vec3(0.0f, 0.0f, 0.01f - move_dist);
  Point3 target = Vec3(0.0f, 0.0f, 15.0f + move_dist);
  // fovy変化: 増加は 0.0 ~ 0.15, 減少は 0.95 ~ 1.0
  float fovy_add_t = t / 0.15f;
  float fovy_dec_t = (t - 0.95f) / 0.05f;
  float fovy = t < 0.15f ? Lerp(40.0f, 65.0f, fovy_add_t) : t < 0.95f ? 65.0f : Lerp(65.0f, 40.0f, EaseOutCubic(fovy_dec_t));
  float time = 0.0f;
  CameraParam param(origin, target, aspect, fovy, time, RandSeed());
  queue.writeBuffer(uniform_buffer_, 0, &param, sizeof(CameraParam));
}
