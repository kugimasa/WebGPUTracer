#include "objects/box.h"

Box::Box(Vec3 aabb_min, Vec3 aabb_max, Color3 color, float rotation, bool emissive) {
  aabb_min_ = aabb_min;
  aabb_max_ = aabb_max;
  center_ = (aabb_max_ + aabb_min_) / 2.0f;
  rotation_ = rotation;
  color_ = color;
  emissive_ = emissive;
  // Rotate Y
  auto min = Point3(fminf(aabb_min_.X(), aabb_max_.X()), fminf(aabb_min_.Y(), aabb_max_.Y()), fminf(aabb_min_.Z(), aabb_max_.Z()));
  auto max = Point3(fmaxf(aabb_min_.X(), aabb_max_.X()), fmaxf(aabb_min_.Y(), aabb_max_.Y()), fmaxf(aabb_min_.Z(), aabb_max_.Z()));
  auto dx = Vec3(max.X() - min.X(), 0, 0);
  auto dy = Vec3(0, max.Y() - min.Y(), 0);
  auto dz = Vec3(0, 0, max.Z() - min.Z());
  quads_.emplace_back(Point3(min.X(), min.Y(), max.Z()), dx, dy, color);
  quads_.emplace_back(Point3(max.X(), min.Y(), max.Z()), -dz, dy, color);
  quads_.emplace_back(Point3(max.X(), min.Y(), min.Z()), -dx, dy, color);
  quads_.emplace_back(Point3(min.X(), min.Y(), min.Z()), dz, dy, color);
  quads_.emplace_back(Point3(min.X(), max.Y(), max.Z()), dx, -dz, color);
  quads_.emplace_back(Point3(min.X(), min.Y(), min.Z()), dx, dz, color);;
}

void Box::PushQuads(std::vector<Quad> &quads) {
  quads.insert(quads.end(), quads_.begin(), quads_.end());
}