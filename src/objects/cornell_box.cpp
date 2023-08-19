#include "objects/cornell_box.h"

CornellBox::CornellBox(Point3 center, Vec3 scale) {
  center_ = center;
  scale_ = scale;
  auto p_x = center_.X();
  auto p_y = center_.Y();
  auto p_z = center_.Z();
  auto s_x = scale_.X() / 2.0f;
  auto s_y = scale_.Y() / 2.0f;
  auto s_z = scale_.Z() / 2.0f;
  auto red = Color3(0.65, 0.05, 0.05);
  auto white = Color3(0.73, 0.73, 0.73);
  auto green = Color3(0.12, 0.45, 0.15);
  /// Bottom
  auto pos = Vec3(p_x, p_y - s_y, p_z);
  auto right = Vec3(s_x, 0, 0);
  auto up = Vec3(0, 0, -s_z);
  quads_.emplace_back(pos, right, up, white);
  /// Top
  pos = Vec3(p_x, p_y + s_y, p_z);
  right = Vec3(s_x, 0, 0);
  up = Vec3(0, 0, s_z);
  quads_.emplace_back(pos, right, up, white);
  /// Right
  pos = Vec3(p_x + s_x, p_y, p_z);
  right = Vec3(0, 0, s_z);
  up = Vec3(0, s_y, 0);
  quads_.emplace_back(pos, right, up, green);
  /// Left
  pos = Vec3(p_x - s_x, p_y, p_z);
  right = Vec3(0, 0, -s_z);
  up = Vec3(0, s_y, 0);
  quads_.emplace_back(pos, right, up, red);
  /// Far
  pos = Vec3(p_x, p_y, p_z - s_z);
  right = Vec3(s_x, 0, 0);
  up = Vec3(0, s_y, 0);
  quads_.emplace_back(pos, right, up, white);
}

void CornellBox::PushToQuads(std::vector<Quad> &quads) {
  quads.insert(quads.end(), quads_.begin(), quads_.end());
}
