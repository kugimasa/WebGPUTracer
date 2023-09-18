#include "objects/box.h"

Box::Box(vec3 aabb_min, vec3 aabb_max, Color3 color, float rotation, bool emissive) {
  aabb_min_ = aabb_min;
  aabb_max_ = aabb_max;
  center_ = (aabb_max_ + aabb_min_) / 2.0f;
  rotation_ = rotation;
  color_ = color;
  emissive_ = emissive;
  // Rotate Y
  auto min = Point3(fminf(aabb_min_.x, aabb_max_.x), fminf(aabb_min_.y, aabb_max_.y), fminf(aabb_min_.z, aabb_max_.z));
  auto max = Point3(fmaxf(aabb_min_.x, aabb_max_.x), fmaxf(aabb_min_.y, aabb_max_.y), fmaxf(aabb_min_.z, aabb_max_.z));
  auto dx = vec3(max.x - min.x, 0, 0);
  auto dy = vec3(0, max.y - min.y, 0);
  auto dz = vec3(0, 0, max.z - min.z);
  quads_.emplace_back(Point3(min.x, min.y, max.z), dx, dy, color);
  quads_.emplace_back(Point3(max.x, min.y, max.z), -dz, dy, color);
  quads_.emplace_back(Point3(max.x, min.y, min.z), -dx, dy, color);
  quads_.emplace_back(Point3(min.x, min.y, min.z), dz, dy, color);
  quads_.emplace_back(Point3(min.x, max.y, max.z), dx, -dz, color);
  quads_.emplace_back(Point3(min.x, min.y, min.z), dx, dz, color);;
}

void Box::PushQuads(std::vector<Quad> &quads) {
  quads.insert(quads.end(), quads_.begin(), quads_.end());
}