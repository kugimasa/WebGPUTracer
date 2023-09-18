#include "objects/quad.h"

Quad::Quad(vec3 center, vec3 right, vec3 up, Color3 color, bool emissive) {
  // FIXME: center_は左下
  center_ = center;
  right_ = right;
  up_ = up;
  auto n = glm::cross(right_, up_);
  norm_ = glm::normalize(n);
  d_ = glm::dot(norm_, center_);
  w_ = n / glm::dot(n, n);
  color_ = color;
  emissive_ = emissive;
}