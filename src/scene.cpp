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
  /// FIXME: 以下テスト用
  auto pos = Point3(0, 0, -9);
  auto scale = Vec3(6, 6, 6);
  auto cb = CornellBox(pos, scale);
  auto u = 1.0f;
  auto v = 2.0f;
  auto center = Point3(2.0f, 0.0f, -9.0f);
  pos = Point3(center.X(), center.Y() - v / 2.0f, center.Z() + u / 2.0f);
  auto right = Vec3(0, 0, -u);
  auto up = Vec3(0, v, 0);
  quads_.emplace_back(pos, right, up, Color3(0.0, 0.2, 0.5));
  pos = Vec3(-2, -2, -10);
  right = Vec3(4, 0, 0);
  up = Vec3(0, 4, 0);
  quads_.emplace_back(pos, right, up, Color3(0.0, 0.2, .05));
  /// CornellBoxの追加
  cb.PushToQuads(quads_);

  /// Sphereの追加
  spheres_.emplace_back(Point3(0, 0, -9), 0.5, Color3(20, 20, 20), true);
  spheres_.emplace_back(Point3(-1.0, 0, -9), 0.3, Color3(0.0, 0.2, 0.5));
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
  quad_buffer_.destroy();
  quad_buffer_.release();
  sphere_buffer_.destroy();
  sphere_buffer_.release();
  storage_.bind_group_layout_.release();
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
      Error(PrintInfoType::Portracer, "TinyObjReader: ", reader.Error());
    }
    exit(1);
  }

  if (!reader.Warning().empty()) {
    Print(PrintInfoType::Portracer, "TinyObjReader: ", reader.Warning());
  }
  tinyobj::attrib_t attrib = reader.GetAttrib();
  std::vector<tinyobj::shape_t> shapes = reader.GetShapes();
  std::vector<tinyobj::material_t> materials = reader.GetMaterials();
  // 頂点の登録
  for (size_t s = 0; s < shapes.size(); ++s) {
    // ポリゴンでループ
    size_t index_offset = 0;
    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
      size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

      // ポリゴン内の頂点でループ
      for (size_t v = 0; v < fv; ++v) {
        // 頂点取得
        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
        auto vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
        auto vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
        auto vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
        auto nx = 0;
        auto ny = 0;
        auto nz = 1;
        auto tx = 0;
        auto ty = 0;

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
  std::vector<BindGroupLayoutEntry> bindings(2, Default);
  /// Scene: Quads
  bindings[0].binding = 0;
  bindings[0].buffer.type = BufferBindingType::ReadOnlyStorage;
  bindings[0].visibility = ShaderStage::Compute;
  /// Scene: Sphere
  bindings[1].binding = 1;
  bindings[1].buffer.type = BufferBindingType::ReadOnlyStorage;
  bindings[1].visibility = ShaderStage::Compute;
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
  quad_buffer_ = CreateQuadBuffer(device);
  sphere_buffer_ = CreateSphereBuffer(device);
}

/*
 * TriangleBufferの作成
 */
Buffer Scene::CreateTriangleBuffer(Device &device) {
  const uint32_t tri_stride = 20 * 4;
  BufferDescriptor tri_buffer_desc{};
  tri_buffer_size_ = tri_stride * tris_.size();
  tri_buffer_desc.size = tri_buffer_size_;
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
  const uint32_t quad_stride = 24 * 4;
  BufferDescriptor quad_buffer_desc{};
  quad_buffer_size_ = quad_stride * quads_.size();
  quad_buffer_desc.size = quad_buffer_size_;
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
Buffer Scene::CreateSphereBuffer(Device &device) {
  const uint32_t sphere_stride = 8 * 4;
  BufferDescriptor sphere_buffer_desc{};
  sphere_buffer_size_ = sphere_stride * spheres_.size();
  sphere_buffer_desc.size = quad_buffer_size_;
  sphere_buffer_desc.usage = BufferUsage::Storage;
  sphere_buffer_desc.mappedAtCreation = true;
  Buffer sphere_buffer = device.createBuffer(sphere_buffer_desc);
  const uint32_t offset = 0;
  const uint32_t size = 0;
  auto *sphere_data = (float *) sphere_buffer.getConstMappedRange(offset, size);
  uint32_t sphere_offset = 0;
  for (int idx = 0; idx < (int) spheres_.size(); ++idx) {
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
  return sphere_buffer;
}

/*
 * BindGroupの初期化
 */
void Scene::InitBindGroup(Device &device) {
  /// BindGroup を作成
  std::vector<BindGroupEntry> entries(2, Default);
  /// QuadBuffer
  entries[0].binding = 0;
  entries[0].buffer = quad_buffer_;
  entries[0].offset = 0;
  entries[0].size = quad_buffer_size_;
  /// SphereBuffer
  entries[1].binding = 1;
  entries[1].buffer = sphere_buffer_;
  entries[1].offset = 0;
  entries[1].size = sphere_buffer_size_;
  BindGroupDescriptor bind_group_desc;
  bind_group_desc.layout = storage_.bind_group_layout_;
  bind_group_desc.entryCount = (uint32_t) entries.size();
  bind_group_desc.entries = (WGPUBindGroupEntry *) entries.data();
  storage_.bind_group_ = device.createBindGroup(bind_group_desc);
}
