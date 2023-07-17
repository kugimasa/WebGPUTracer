#include "objects/cornell_box.h"
#include "objects/box.h"

CornellBox::CornellBox(Device &device) {
  /// Initialize Scene
//  Box walls = Box(Vec3(0, 5, 0), Vec3(10, 10, 10), 0, Color3(0.5, 0.0, 0.0), false);
//  Box box1 = Box(Vec3(1.5, 1.5, 1), Vec3(3, 3, 3), 0.3, Color3(0.8, 0.8, 0.8));
//  Box box2 = Box(Vec3(-2, 3, -2), Vec3(3, 6, 3), -0.4, Color3(0.8, 0.8, 0.8));
//  Box box = Box(Vec3(0, 0, 5.0), Vec3(3, 3, 3), 0.0, Color3(0.8, 0.8, 0.8), false, false);
  /// Initialize Light
  Quad light = Quad(Vec3(0, 0, 9.5), Vec3(1, 0, 0), Vec3(0, 1, 0), Color3(5.0, 5.0, 5.0), true);
  /// Create Quad Buffer
//  walls.PushQuads(quads_);
//  box1.PushQuads(quads_);
//  box2.PushQuads(quads_);
//  box.PushQuads(quads_);
  quads_.push_back(light);
  const uint32_t quad_stride = 16 * 4;
  BufferDescriptor quad_buffer_desc{};
  quad_buffer_desc.size = quad_stride * quads_.size();
  quad_buffer_desc.usage = BufferUsage::Storage;
  quad_buffer_desc.mappedAtCreation = true;
  Buffer quad_buffer = device.createBuffer(quad_buffer_desc);
  const uint32_t offset = 0;
  const uint32_t size = 0;
  auto *quad_data = (float *) quad_buffer.getConstMappedRange(offset, size);
  const uint32_t vertex_stride = 4 * 10;
  std::vector<float> vertex_data(quads_.size() * vertex_stride);
  std::vector<uint16_t> index_data(quads_.size() * 6);
  uint32_t quad_offset = 0;
  uint32_t vertex_offset = 0;
  uint32_t index_offset = 0;
  uint32_t vertex_count = 0;
  uint32_t index_count = 0;
  for (int idx = 0; idx < (int) quads_.size(); ++idx) {
    Quad quad = quads_[idx];
    int emissive = quad.emissive_ ? 1 : 0;
    const Vec3 normal = Unit(Cross(quad.right_, quad.up_));
    quad_data[quad_offset++] = normal[0];
    quad_data[quad_offset++] = normal[1];
    quad_data[quad_offset++] = normal[2];
    const Vec3 inv_right = Inv(quad.right_);
    quad_data[quad_offset++] = inv_right[0];
    quad_data[quad_offset++] = inv_right[1];
    quad_data[quad_offset++] = inv_right[2];
    const Vec3 inv_up = Inv(quad.up_);
    quad_data[quad_offset++] = inv_up[0];
    quad_data[quad_offset++] = inv_up[1];
    quad_data[quad_offset++] = inv_up[2];
    quad_data[quad_offset++] = quad.color_[0];
    quad_data[quad_offset++] = quad.color_[1];
    quad_data[quad_offset++] = quad.color_[2];
    quad_data[quad_offset++] = emissive;

    // a ----- b
    // |       |
    // |   m   |
    // |       |
    // c ----- d
    const Vec3 a = quad.center_ - quad.right_ + quad.up_;
    const Vec3 b = quad.center_ + quad.right_ + quad.up_;
    const Vec3 c = quad.center_ - quad.right_ - quad.up_;
    const Vec3 d = quad.center_ + quad.right_ - quad.up_;

    vertex_data[vertex_offset++] = a[0];
    vertex_data[vertex_offset++] = a[1];
    vertex_data[vertex_offset++] = a[2];
    vertex_data[vertex_offset++] = 1;
    vertex_data[vertex_offset++] = 0; // uv.x
    vertex_data[vertex_offset++] = 1; // uv.y
    vertex_data[vertex_offset++] = idx;
    vertex_data[vertex_offset++] = quad.color_[0] * emissive;
    vertex_data[vertex_offset++] = quad.color_[1] * emissive;
    vertex_data[vertex_offset++] = quad.color_[2] * emissive;

    vertex_data[vertex_offset++] = b[0];
    vertex_data[vertex_offset++] = b[1];
    vertex_data[vertex_offset++] = b[2];
    vertex_data[vertex_offset++] = 1;
    vertex_data[vertex_offset++] = 1; // uv.x
    vertex_data[vertex_offset++] = 1; // uv.y
    vertex_data[vertex_offset++] = idx;
    vertex_data[vertex_offset++] = quad.color_[0] * emissive;
    vertex_data[vertex_offset++] = quad.color_[1] * emissive;
    vertex_data[vertex_offset++] = quad.color_[2] * emissive;

    vertex_data[vertex_offset++] = c[0];
    vertex_data[vertex_offset++] = c[1];
    vertex_data[vertex_offset++] = c[2];
    vertex_data[vertex_offset++] = 1;
    vertex_data[vertex_offset++] = 0; // uv.x
    vertex_data[vertex_offset++] = 0; // uv.y
    vertex_data[vertex_offset++] = idx;
    vertex_data[vertex_offset++] = quad.color_[0] * emissive;
    vertex_data[vertex_offset++] = quad.color_[1] * emissive;
    vertex_data[vertex_offset++] = quad.color_[2] * emissive;

    vertex_data[vertex_offset++] = d[0];
    vertex_data[vertex_offset++] = d[1];
    vertex_data[vertex_offset++] = d[2];
    vertex_data[vertex_offset++] = 1;
    vertex_data[vertex_offset++] = 1; // uv.x
    vertex_data[vertex_offset++] = 0; // uv.y
    vertex_data[vertex_offset++] = idx;
    vertex_data[vertex_offset++] = quad.color_[0] * emissive;
    vertex_data[vertex_offset++] = quad.color_[1] * emissive;
    vertex_data[vertex_offset++] = quad.color_[2] * emissive;

    index_data[index_offset++] = vertex_count + 0; // a
    index_data[index_offset++] = vertex_count + 2; // c
    index_data[index_offset++] = vertex_count + 1; // b
    index_data[index_offset++] = vertex_count + 1; // b
    index_data[index_offset++] = vertex_count + 2; // c
    index_data[index_offset++] = vertex_count + 3; // d
    index_count += 6;
    vertex_count += 4;
  }
  quad_buffer.unmap();
  quad_buffer_ = quad_buffer;

  /// Create VertexBuffer : VERTEX
  BufferDescriptor vertex_buffer_desc{};
  vertex_buffer_desc.size = vertex_data.size() * sizeof(float);
  vertex_buffer_desc.usage = BufferUsage::Vertex;
  vertex_buffer_desc.mappedAtCreation = true;
  Buffer vertices = device.createBuffer(vertex_buffer_desc);
  // TODO: in C++
  // new Float32Array(vertices.getMappedRange()).set(vertexData);
  vertices.unmap();
  vertex_count_ = vertex_count;
  vertices_ = vertices;

  /// Create IndexBuffer : INDEX
  BufferDescriptor index_buffer_desc{};
  index_buffer_desc.size = index_data.size() * sizeof(uint16_t);
  index_buffer_desc.usage = BufferUsage::Index;
  index_buffer_desc.mappedAtCreation = true;
  Buffer indices = device.createBuffer(index_buffer_desc);
  // TODO: in C++
  // new Uint16Array(indices.getMappedRange()).set(indexData);
  indices.unmap();
  index_count_ = index_count;
  indices_ = indices;

  /// VertexBufferLayout
  std::vector<VertexAttribute> vertex_attributes(3);
  // Position
  vertex_attributes[0].shaderLocation = 0;
  vertex_attributes[0].format = VertexFormat::Float32x4;
  vertex_attributes[0].offset = 0 * 4;
  // UV
  vertex_attributes[1].shaderLocation = 1;
  vertex_attributes[1].format = VertexFormat::Float32x3;
  vertex_attributes[1].offset = 4 * 4;
  // Color
  vertex_attributes[2].shaderLocation = 2;
  vertex_attributes[2].format = VertexFormat::Float32x3;
  vertex_attributes[2].offset = 7 * 4;
  vertex_buffer_layout_.arrayStride = vertex_stride;
  vertex_buffer_layout_.attributeCount = static_cast<uint32_t>(vertex_attributes.size());
  vertex_buffer_layout_.attributes = vertex_attributes.data();
}
