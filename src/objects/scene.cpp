#include "objects/scene.h"

/*
 * コンストラクタ
 */
Scene::Scene(Device &device) {
  Vertex v0(Point3(0, 1, -9), Vec3(0, 0, -1), 0, 1);
  Vertex v1(Point3(-3, -1, -9), Vec3(0, 0, -1), 1, 0);
  Vertex v2(Point3(3, -1, -9), Vec3(0, 0, -1), 1, 1);
  Triangle tri(v0, v1, v2, Color3(0, 0, 1));
  /// SceneBufferの作成
  tris_.push_back(tri);
  /// バッファのバインド
  InitBindGroupLayout(device);
  InitBuffers(device);
  InitBindGroup(device);
}

/*
 * シーンの解放
 */
void Scene::Release() {
  storage_.bind_group_.release();
  scene_buffer_.destroy();
  scene_buffer_.release();
  storage_.bind_group_layout_.release();
}

/*
 * BindGroupLayoutの初期化
 */
void Scene::InitBindGroupLayout(Device &device) {
  std::vector<BindGroupLayoutEntry> bindings(1, Default);
  /// Scene: Tris
  bindings[0].binding = 0;
  bindings[0].buffer.type = BufferBindingType::ReadOnlyStorage;
  bindings[0].visibility = ShaderStage::Compute;
  /// BindGroupLayoutの作成
  BindGroupLayoutDescriptor bind_group_layout_desc{};
  bind_group_layout_desc.entryCount = (uint32_t) bindings.size();
  bind_group_layout_desc.entries = bindings.data();
  bind_group_layout_desc.label = "Scene.storage_.bind_group_layout_";
  storage_.bind_group_layout_ = device.createBindGroupLayout(bind_group_layout_desc);
}

/*
 * Buffer作成
 */
void Scene::InitBuffers(Device &device) {
  const uint32_t tri_stride = 20 * 4;
  BufferDescriptor tri_buffer_desc{};
  scene_buffer_size_ = tri_stride * tris_.size();
  tri_buffer_desc.size = scene_buffer_size_;
  tri_buffer_desc.usage = BufferUsage::Storage;
  tri_buffer_desc.mappedAtCreation = true;
  Buffer tri_buffer = device.createBuffer(tri_buffer_desc);
  const uint32_t offset = 0;
  const uint32_t size = 0;
  auto *tri_data = (float *) tri_buffer.getConstMappedRange(offset, size);
  uint32_t tri_offset = 0;
  const float dummy = 1.0f;
  for (int idx = 0; idx < (int) tris_.size(); ++idx) {
    Triangle tri = tris_[idx];
    /// 頂点v0
    const Point3 vertex = tri.vertex_[0].point_;
    tri_data[tri_offset++] = vertex[0];
    tri_data[tri_offset++] = vertex[1];
    tri_data[tri_offset++] = vertex[2];
    tri_data[tri_offset++] = dummy;
    /// ベクトルe1
    const Vec3 e1 = tri.e1_;
    tri_data[tri_offset++] = e1[0];
    tri_data[tri_offset++] = e1[1];
    tri_data[tri_offset++] = e1[2];
    tri_data[tri_offset++] = dummy;
    /// ベクトルe2
    const Vec3 e2 = tri.e2_;
    tri_data[tri_offset++] = e2[0];
    tri_data[tri_offset++] = e2[1];
    tri_data[tri_offset++] = e2[2];
    tri_data[tri_offset++] = dummy;
    /// 面法線
    const Vec3 face_norm = tri.face_norm_;
    tri_data[tri_offset++] = face_norm[0];
    tri_data[tri_offset++] = face_norm[1];
    tri_data[tri_offset++] = face_norm[2];
    tri_data[tri_offset++] = dummy;
    /// カラー
    const Color3 color = tri.color_;
    tri_data[tri_offset++] = color[0];
    tri_data[tri_offset++] = color[1];
    tri_data[tri_offset++] = color[2];
    /// エミッシブ
    const float emissive = tri.emissive_ ? 1.0f : 0.0f;
    tri_data[tri_offset++] = emissive;
  }
  tri_buffer.unmap();
  scene_buffer_ = tri_buffer;
}

/*
 * BindGroupの初期化
 */
void Scene::InitBindGroup(Device &device) {
  /// BindGroup を作成
  std::vector<BindGroupEntry> entries(1, Default);
  /// SceneBuffer
  entries[0].binding = 0;
  entries[0].buffer = scene_buffer_;
  entries[0].offset = 0;
  entries[0].size = scene_buffer_size_;
  BindGroupDescriptor bind_group_desc;
  bind_group_desc.layout = storage_.bind_group_layout_;
  bind_group_desc.entryCount = (uint32_t) entries.size();
  bind_group_desc.entries = (WGPUBindGroupEntry *) entries.data();
  storage_.bind_group_ = device.createBindGroup(bind_group_desc);
}
