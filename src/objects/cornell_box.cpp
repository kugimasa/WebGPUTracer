#include "objects/cornell_box.h"

CornellBox::CornellBox(Point3 center, Vec3 scale) {
  center_ = center;
  scale_ = scale;
  auto p_x = center_.X();
  auto p_y = center_.Y();
  auto p_z = center_.Z();
  auto s_x = scale_.X();
  auto s_y = scale_.Y();
  auto s_z = scale_.Z();
  auto white = Color3(0.73f, 0.73f, 0.73f);
  /// Top
  auto pos = Vec3(p_x - s_x / 2.0f, p_y + s_y / 2.0f, p_z - s_z / 2.0f);
  auto right = Vec3(s_x, 0, 0);
  auto up = Vec3(0, 0, s_z);
  quads_.emplace_back(pos, right, up, white);
  /// Bottom
  pos = Vec3(p_x - s_x / 2.0f, p_y - s_y / 2.0f, p_z + s_z / 2.0f);
  right = Vec3(s_x, 0, 0);
  up = Vec3(0, 0, -s_z);
  quads_.emplace_back(pos, right, up, white);
  /// Right
  pos = Vec3(p_x + s_x / 2.0f, p_y - s_y / 2.0f, p_z - s_z / 2.0f);
  right = Vec3(0, 0, s_z);
  up = Vec3(0, s_y, 0);
  quads_.emplace_back(pos, right, up, white);
  /// Left
  pos = Vec3(p_x - s_x / 2.0f, p_y - s_y / 2.0f, p_z + s_z / 2.0f);
  right = Vec3(0, 0, -s_z);
  up = Vec3(0, s_y, 0);
  quads_.emplace_back(pos, right, up, white);
  /// Far
  pos = Vec3(p_x - s_x / 2.0f, p_y - s_y / 2.0f, p_z - s_z / 2.0f);
  right = Vec3(s_x, 0, 0);
  up = Vec3(0, s_y, 0);
  quads_.emplace_back(pos, right, up, white);
}

void CornellBox::PushToQuads(std::vector<Quad> &quads) {
  quads.insert(quads.end(), quads_.begin(), quads_.end());
}
