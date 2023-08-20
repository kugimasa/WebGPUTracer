#include "objects/quad.h"

Quad::Quad(Vec3 center, Vec3 right, Vec3 up, Color3 color, bool emissive) {
  // FIXME: center_は左下
  center_ = center;
  right_ = right;
  up_ = up;
  auto n = Cross(right_, up_);
  norm_ = Unit(n);
  d_ = Dot(norm_, center_);
  w_ = n / Dot(n, n);
  color_ = color;
  emissive_ = emissive;
}