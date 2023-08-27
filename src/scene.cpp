#include "scene.h"
#include "objects/cornell_box.h"

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
// Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust trinagulation. Requires C++11
//#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"

/*
 * コンストラクタ
 */
Scene::Scene(Device &device) {
  /// Quadの追加
  auto pos = Point3(0.0f, 0.0f, 0.0f);
  auto scale = Vec3(8.0f, 5.0f, 500.0f);
  auto cb = CornellBox(pos, scale);
  /// CornellBoxの追加
  cb.PushToQuads(quads_);

  /// Sphereの追加
  auto white = Color3(0.73f, 0.73f, 0.73f);
  spheres_.emplace_back(Point3(-1.0f, 0.0f, -10.0f), 0.3f, white);
  spheres_.emplace_back(Point3(0.0f, 1.0f, -20.0f), 0.3f, white);
  spheres_.emplace_back(Point3(1.0f, 0.0f, -30.0f), 0.3f, white);
  spheres_.emplace_back(Point3(0.0f, -1.0f, -40.0f), 0.3f, white);
  spheres_.emplace_back(Point3(-1.0f, 0.0f, -50.0f), 0.3f, white);
  spheres_.emplace_back(Point3(0.0f, 1.0f, -60.0f), 0.3f, white);
  spheres_.emplace_back(Point3(1.0f, 0.0f, -70.0f), 0.3f, white);
  spheres_.emplace_back(Point3(0.0f, -1.0f, -80.0f), 0.3f, white);
  spheres_.emplace_back(Point3(-1.0f, 0.0f, -90.0f), 0.3f, white);
  spheres_.emplace_back(Point3(0.0f, 1.0f, -100.0f), 0.3f, white);
  spheres_.emplace_back(Point3(1.0f, 0.0f, -110.0f), 0.3f, white);
  spheres_.emplace_back(Point3(0.0f, -1.0f, -120.0f), 0.3f, white);
  InitBindGroupLayout(device);
  InitBuffers(device);
  InitBindGroup(device);
}

/*
 * シーンの解放
 */
void Scene::Release() {
  objects_.bind_group_.release();
  quad_buffer_.destroy();
  quad_buffer_.release();
  sphere_buffer_.destroy();
  sphere_buffer_.release();
  sphere_light_buffer_.destroy();
  sphere_light_buffer_.release();
  objects_.bind_group_layout_.release();
}


/*
 * Objファイルのロード
 */
void Scene::LoadObj(const char *file_path, Color3 color, Vec3 translation, bool emissive) {
  std::vector<Vertex> vertices;
  LoadVertices(file_path, vertices);
  for (size_t i = 0; i < vertices.size() / 3; ++i) {
    auto v0 = vertices[i * 3].Translate(translation);
    auto v1 = vertices[i * 3 + 1].Translate(translation);
    auto v2 = vertices[i * 3 + 2].Translate(translation);
    tris_.emplace_back(v0, v1, v2, color, emissive);
  }
}

/*
 * Objファイルから頂点リストをロード
 */
void Scene::LoadVertices(const char *file_path, std::vector<Vertex> &vertices) {
  tinyobj::ObjReaderConfig reader_config;
  tinyobj::ObjReader reader;
  reader_config.mtl_search_path = "./assets/obj/";
  if (!reader.ParseFromFile(file_path, reader_config)) {
    if (!reader.Error().empty()) {
      Error(PrintInfoType::WebGPUTracer, "TinyObjReader: ", reader.Error());
    }
    exit(1);
  }

  if (!reader.Warning().empty()) {
    Print(PrintInfoType::WebGPUTracer, "TinyObjReader: ", reader.Warning());
  }
  tinyobj::attrib_t attrib = reader.GetAttrib();
  std::vector<tinyobj::shape_t> shapes = reader.GetShapes();
  std::vector<tinyobj::material_t> materials = reader.GetMaterials();
  // 頂点の登録
  for (size_t s = 0; s < shapes.size(); ++s) {
    // ポリゴンでループ
    size_t index_offset = 0;
    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
      auto fv = size_t(shapes[s].mesh.num_face_vertices[f]);

      // ポリゴン内の頂点でループ
      for (size_t v = 0; v < fv; ++v) {
        // 頂点取得
        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
        auto vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
        auto vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
        auto vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
        auto nx = 0.0f;
        auto ny = 0.0f;
        auto nz = 1.0f;
        auto tx = 0.0f;
        auto ty = 0.0f;

        // 法線判定
        if (idx.normal_index >= 0) {
          nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
          ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
          nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
        }

        // テクスチャ座標
        if (idx.texcoord_index >= 0) {
          tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
          ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
        }

        Vertex vert;
        vert.point_ = Vec3(vx, vy, vz);
        vert.normal_ = Vec3(nx, ny, nz);
        vert.u_ = tx;
        vert.v_ = ty;
        vertices.push_back(vert);
      }
      index_offset += fv;
    }
  }
  shapes.clear();
}

/*
 * BindGroupLayoutの初期化
 */
void Scene::InitBindGroupLayout(Device &device) {
  std::vector<BindGroupLayoutEntry> bindings(3, Default);
  /// Scene: Quads
  bindings[0].binding = 0;
  bindings[0].buffer.type = BufferBindingType::ReadOnlyStorage;
  bindings[0].visibility = ShaderStage::Compute;
  /// Scene: Spheres
  bindings[1].binding = 1;
  bindings[1].buffer.type = BufferBindingType::ReadOnlyStorage;
  bindings[1].visibility = ShaderStage::Compute;
  /// Scene: Sphere Lights
  bindings[2].binding = 2;
  bindings[2].buffer.type = BufferBindingType::Uniform;
  bindings[2].visibility = ShaderStage::Compute;
  /// BindGroupLayoutの作成
  BindGroupLayoutDescriptor bind_group_layout_desc{};
  bind_group_layout_desc.entryCount = (uint32_t) bindings.size();
  bind_group_layout_desc.entries = bindings.data();
  bind_group_layout_desc.label = "Scene.objects_.bind_group_layout_";
  objects_.bind_group_layout_ = device.createBindGroupLayout(bind_group_layout_desc);
}

/*
 * Buffer作成
 */
void Scene::InitBuffers(Device &device) {
  quad_buffer_ = CreateQuadBuffer(device);
  sphere_buffer_ = CreateSphereBuffer(device, spheres_.size(), BufferUsage::Storage, true);
  sphere_light_buffer_ = CreateSphereBuffer(device, GetSphereLightsNum(), BufferUsage::Uniform | BufferUsage::CopyDst, false);
}

/*
 * TriangleBufferの作成
 */
Buffer Scene::CreateTriangleBuffer(Device &device) {
  BufferDescriptor tri_buffer_desc{};
  auto tri_buffer_size = tri_stride_ * tris_.size();
  tri_buffer_desc.size = tri_buffer_size;
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
    tri_data[tri_offset++] = tri.emissive_;
  }
  tri_buffer.unmap();
  return tri_buffer;
}

/*
 * QuadBufferの作成
 */
Buffer Scene::CreateQuadBuffer(Device &device) {
  BufferDescriptor quad_buffer_desc{};
  auto quad_buffer_size = quad_stride_ * quads_.size();
  quad_buffer_desc.size = quad_buffer_size;
  quad_buffer_desc.usage = BufferUsage::Storage;
  quad_buffer_desc.mappedAtCreation = true;
  Buffer quad_buffer = device.createBuffer(quad_buffer_desc);
  const uint32_t offset = 0;
  const uint32_t size = 0;
  auto *quad_data = (float *) quad_buffer.getConstMappedRange(offset, size);
  uint32_t quad_offset = 0;
  const float dummy = 1.0f;
  for (int idx = 0; idx < (int) quads_.size(); ++idx) {
    Quad quad = quads_[idx];
    /// 位置
    quad_data[quad_offset++] = quad.center_[0];
    quad_data[quad_offset++] = quad.center_[1];
    quad_data[quad_offset++] = quad.center_[2];
    quad_data[quad_offset++] = dummy;
    /// 右ベクトル
    quad_data[quad_offset++] = quad.right_[0];
    quad_data[quad_offset++] = quad.right_[1];
    quad_data[quad_offset++] = quad.right_[2];
    quad_data[quad_offset++] = dummy;
    /// 上ベクトル
    quad_data[quad_offset++] = quad.up_[0];
    quad_data[quad_offset++] = quad.up_[1];
    quad_data[quad_offset++] = quad.up_[2];
    quad_data[quad_offset++] = dummy;
    /// 法線
    quad_data[quad_offset++] = quad.norm_[0];
    quad_data[quad_offset++] = quad.norm_[1];
    quad_data[quad_offset++] = quad.norm_[2];
    quad_data[quad_offset++] = dummy;
    /// W = n / dot(n, n)
    quad_data[quad_offset++] = quad.w_[0];
    quad_data[quad_offset++] = quad.w_[1];
    quad_data[quad_offset++] = quad.w_[2];
    /// D = n_x q_x + n_y q_y + n_z q_z
    quad_data[quad_offset++] = quad.d_;
    /// カラー
    quad_data[quad_offset++] = quad.color_[0];
    quad_data[quad_offset++] = quad.color_[1];
    quad_data[quad_offset++] = quad.color_[2];
    /// エミッシブ
    quad_data[quad_offset++] = quad.emissive_;
  }
  quad_buffer.unmap();
  return quad_buffer;
}

/*
 * SphereBufferの作成
 */
Buffer Scene::CreateSphereBuffer(Device &device, size_t num, WGPUBufferUsageFlags usage_flags, bool mapped_at_creation) {
  BufferDescriptor sphere_buffer_desc{};
  auto sphere_buffer_size = sphere_stride_ * num;
  sphere_buffer_desc.size = sphere_buffer_size;
  sphere_buffer_desc.usage = usage_flags;
  sphere_buffer_desc.mappedAtCreation = mapped_at_creation;
  Buffer sphere_buffer = device.createBuffer(sphere_buffer_desc);
  if (mapped_at_creation) {
    const uint32_t offset = 0;
    const uint32_t size = 0;
    auto *sphere_data = (float *) sphere_buffer.getConstMappedRange(offset, size);
    uint32_t sphere_offset = 0;
    for (uint32_t idx = 0; idx < spheres_.size(); ++idx) {
      Sphere sphere = spheres_[idx];
      /// 中心
      sphere_data[sphere_offset++] = sphere.center_[0];
      sphere_data[sphere_offset++] = sphere.center_[1];
      sphere_data[sphere_offset++] = sphere.center_[2];
      /// 半径
      sphere_data[sphere_offset++] = sphere.radius_;
      /// カラー
      sphere_data[sphere_offset++] = sphere.color_[0];
      sphere_data[sphere_offset++] = sphere.color_[1];
      sphere_data[sphere_offset++] = sphere.color_[2];
      /// エミッシブ
      sphere_data[sphere_offset++] = sphere.emissive_;
    }
    sphere_buffer.unmap();
  }
  return sphere_buffer;
}

/*
 * BindGroupの初期化
 */
void Scene::InitBindGroup(Device &device) {
  /// BindGroup を作成
  std::vector<BindGroupEntry> entries(3, Default);
  /// QuadBuffer
  entries[0].binding = 0;
  entries[0].buffer = quad_buffer_;
  entries[0].offset = 0;
  entries[0].size = quad_stride_ * quads_.size();
  /// SphereBuffer
  entries[1].binding = 1;
  entries[1].buffer = sphere_buffer_;
  entries[1].offset = 0;
  entries[1].size = sphere_stride_ * spheres_.size();
  /// SphereLightBuffer
  entries[2].binding = 2;
  entries[2].buffer = sphere_light_buffer_;
  entries[2].offset = 0;
  entries[2].size = sizeof(SphereLights);
  BindGroupDescriptor bind_group_desc;
  bind_group_desc.layout = objects_.bind_group_layout_;
  bind_group_desc.entryCount = (uint32_t) entries.size();
  bind_group_desc.entries = (WGPUBindGroupEntry *) entries.data();
  objects_.bind_group_ = device.createBindGroup(bind_group_desc);
}

/*
 * SphereLightの更新
 */
void Scene::UpdateSphereLights(Queue &queue, float t) const {
  float cam_pos = t < 0.2f ? 8.0f * EaseInQuart(t / 0.2f) : 8.0f * EaseInQuart((t - 0.2f) / 0.8f + 1.0f);
  float dist_from_cam = 5.0f;
  float move_dist = cam_pos + dist_from_cam;
  bool l1_on = move_dist > 15.0f - dist_from_cam;
  bool l2_on = move_dist > 30.0f - dist_from_cam;
  bool l3_on = move_dist > 45.0f - dist_from_cam;
  bool l4_on = move_dist > 60.0f - dist_from_cam;
  bool l5_on = move_dist > 75.0f - dist_from_cam;
  bool l6_on = move_dist > 90.0f - dist_from_cam;
  bool l7_on = move_dist > 105.0f - dist_from_cam;
  float light_power = t < 0.15f ? Lerp(0.0f, 1000.0f, t / 0.15f) : t < 0.95f ? 1000.0f : Lerp(1000.0f, 0.0f, EaseOutCubic((t - 0.95f) / 0.05f));
  Color3 light_off_col = Color3(0.2f, 0.2f, 0.2f);
  Color3 move_light_col = light_power * Color3(1.0f, 1.0f, 1.0f);
  Color3 col1 = l1_on ? light_power * Color3(255.0f / 255.0f, 0.0f, 0.0f) : light_off_col;
  Color3 col2 = l2_on ? light_power * Color3(255.0f / 255.0f, 127.0f / 255.0f, 0.0f) : light_off_col;
  Color3 col3 = l3_on ? light_power * Color3(255.0f / 255.0f, 255.0f / 255.0f, 0.0f) : light_off_col;
  Color3 col4 = l4_on ? light_power * Color3(0.0f, 255.0f / 255.0f, 0.0f) : light_off_col;
  Color3 col5 = l5_on ? light_power * Color3(0.0f, 0.0f, 255.0f / 255.0f) : light_off_col;
  Color3 col6 = l6_on ? light_power * Color3(75.0f / 255.0f, 0.0f, 130.0f / 255.0f) : light_off_col;
  Color3 col7 = l7_on ? light_power * Color3(148.0f / 255.0f, 0.0f, 211.0f / 255.0f) : light_off_col;
  // 移動ライトの色を変化
  if (l7_on) {
    move_light_col = col7;
  } else if (l6_on) {
    move_light_col = col6;
  } else if (l5_on) {
    move_light_col = col5;
  } else if (l4_on) {
    move_light_col = col4;
  } else if (l3_on) {
    move_light_col = col3;
  } else if (l2_on) {
    move_light_col = col2;
  } else if (l1_on) {
    move_light_col = col1;
  }
  Sphere l1(Point3(0.0f, 2.0f, -15.0f), 0.3f, col1, l1_on);
  Sphere l2(Point3(0.0f, 2.0f, -30.0f), 0.3f, col2, l2_on);
  Sphere l3(Point3(0.0f, 2.0f, -45.0f), 0.3f, col3, l3_on);
  Sphere l4(Point3(0.0f, 2.0f, -60.0f), 0.3f, col4, l4_on);
  Sphere l5(Point3(0.0f, 2.0f, -75.0f), 0.3f, col5, l5_on);
  Sphere l6(Point3(0.0f, 2.0f, -90.0f), 0.3f, col6, l6_on);
  Sphere l7(Point3(0.0f, 2.0f, -105.0f), 0.3f, col7, l7_on);
  float theta = cam_pos / 12.0f * (float) (M_PI * 2.0f);
  float x = cos(theta);
  float y = sin(theta);
  Point3 origin = Vec3(x, y, 0.01f - move_dist);
  Sphere move_l(origin, 0.1f, move_light_col, true);
  SphereLights lights(l1, l2, l3, l4, l5, l6, l7, move_l);
  queue.writeBuffer(sphere_light_buffer_, 0, &lights, sizeof(SphereLights));
}
