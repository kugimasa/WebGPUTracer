#include "objects/scene.h"

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
// Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust trinagulation. Requires C++11
//#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"

/*
 * コンストラクタ
 */
Scene::Scene(Device &device) {
  Vertex v0(Point3(0, 1, -9), Vec3(0, 0, -1), 0, 1);
  Vertex v1(Point3(-3, -1, -9), Vec3(0, 0, -1), 1, 0);
  Vertex v2(Point3(3, -1, -9), Vec3(0, 0, -1), 1, 1);
  Triangle tri(v0, v1, v2, Color3(0, 0, 1));
  v0 = Vertex(Point3(2, 1, -10), Vec3(0, 0, -1), 0, 1);
  v1 = Vertex(Point3(-3, -1, -10), Vec3(0, 0, -1), 1, 0);
  v2 = Vertex(Point3(3, -1, -10), Vec3(0, 0, -1), 1, 1);
  Triangle tri2(v0, v1, v2, Color3(0, 1, 0));
  tris_.push_back(tri);
  tris_.push_back(tri2);
  /// テスト用のQuad
  auto center = Vec3(0, 0, -10);
  auto right = Vec3(1, 0, 0);
  auto up = Vec3(0, 1, 0);
  Quad test_quad = Quad(center, right, up, Color3(1, 0, 0), true);
  /// Quad Bufferの作成
  quads_.push_back(test_quad);
  // LoadObj(RESOURCE_DIR "/obj/kugizarashi.obj", Color3(0.5, 0.5, 0.5));
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
  tri_buffer_.destroy();
  tri_buffer_.release();
  quad_buffer_.destroy();
  quad_buffer_.release();
  storage_.bind_group_layout_.release();
}


/*
 * Objファイルのロード
 */
void Scene::LoadObj(const char *file_path, Color3 color) {
  std::vector<Vertex> vertices;
  LoadVertices(file_path, vertices);
  for (size_t i = 0; i < vertices.size() / 3; ++i) {
    tris_.emplace_back(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2], color);
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
  /// Scene: Tris
  bindings[0].binding = 0;
  bindings[0].buffer.type = BufferBindingType::ReadOnlyStorage;
  bindings[0].visibility = ShaderStage::Compute;
  // Scene: Quads
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
  tri_buffer_ = CreateTriangleBuffer(device);
  quad_buffer_ = CreateQuadBuffer(device);
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
    const float emissive = tri.emissive_ ? 1.0f : 0.0f;
    tri_data[tri_offset++] = emissive;
  }
  tri_buffer.unmap();
  return tri_buffer;
}

/*
 * QuadBufferの作成
 */
Buffer Scene::CreateQuadBuffer(Device &device) {
  const uint32_t quad_stride = 16 * 4;
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
  for (int idx = 0; idx < (int) quads_.size(); ++idx) {
    Quad quad = quads_[idx];
    const Vec3 normal = Unit(Cross(quad.right_, quad.up_));
    quad_data[quad_offset++] = normal[0];
    quad_data[quad_offset++] = normal[1];
    quad_data[quad_offset++] = normal[2];
    quad_data[quad_offset++] = -Dot(normal, quad.center_);
    const Vec3 inv_right = Inv(quad.right_);
    quad_data[quad_offset++] = inv_right[0];
    quad_data[quad_offset++] = inv_right[1];
    quad_data[quad_offset++] = inv_right[2];
    quad_data[quad_offset++] = -Dot(inv_right, quad.center_);
    const Vec3 inv_up = Inv(quad.up_);
    quad_data[quad_offset++] = inv_up[0];
    quad_data[quad_offset++] = inv_up[1];
    quad_data[quad_offset++] = inv_up[2];
    quad_data[quad_offset++] = -Dot(inv_up, quad.center_);
    quad_data[quad_offset++] = quad.color_[0];
    quad_data[quad_offset++] = quad.color_[1];
    quad_data[quad_offset++] = quad.color_[2];
    const float emissive = quad.emissive_ ? 1.0f : 0.0f;
    quad_data[quad_offset++] = emissive;
  }
  quad_buffer.unmap();
  return quad_buffer;
}

/*
 * BindGroupの初期化
 */
void Scene::InitBindGroup(Device &device) {
  /// BindGroup を作成
  std::vector<BindGroupEntry> entries(2, Default);
  /// TriBuffer
  entries[0].binding = 0;
  entries[0].buffer = tri_buffer_;
  entries[0].offset = 0;
  entries[0].size = tri_buffer_size_;
  /// QuadBuffer
  entries[1].binding = 1;
  entries[1].buffer = quad_buffer_;
  entries[1].offset = 0;
  entries[1].size = quad_buffer_size_;
  BindGroupDescriptor bind_group_desc;
  bind_group_desc.layout = storage_.bind_group_layout_;
  bind_group_desc.entryCount = (uint32_t) entries.size();
  bind_group_desc.entries = (WGPUBindGroupEntry *) entries.data();
  storage_.bind_group_ = device.createBindGroup(bind_group_desc);
}
