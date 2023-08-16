#include "objects/cornell_box.h"
#include "objects/box.h"

CornellBox::CornellBox(Device &device) {
//  /// Initialize Scene
//  Box walls = Box(Vec3(0, 5, 0), Vec3(10, 10, 10), 0, Color3(1, 0, 0));
//  Box box1 = Box(Vec3(1.5, 1.5, 1), Vec3(3, 3, 3), 0.3, Color3(0, 1, 0));
//  Box box2 = Box(Vec3(-2, 3, -2), Vec3(3, 6, 3), -0.4, Color3(0, 0, 1));
//  /// Initialize Light
//  Quad light = Quad(Vec3(0, 0, 9.5), Vec3(1, 0, 0), Vec3(0, 1, 0), Color3(5.0, 5.0, 5.0), true);
  /// テスト用のQuad
  auto center = Vec3(0, 0, -10);
  auto right = Vec3(1, 0, 0);
  auto up = Vec3(0, 1, 0);
  Quad test_quad = Quad(center, right, up, Color3(1, 0, 0), true);
  /// Quad Bufferの作成
  quads_.push_back(test_quad);
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
  for (int idx = 0; idx < (int) quads_.size(); ++idx) {
    Quad quad = quads_[idx];
    int emissive = quad.emissive_ ? 1 : 0;
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
    quad_data[quad_offset++] = emissive;
  }
  quad_buffer.unmap();
  quad_buffer_ = quad_buffer;
}
