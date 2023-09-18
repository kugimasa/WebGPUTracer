#include "objects/quad.h"

Quad::Quad(vec3 q, vec3 right, vec3 up, Color3 color, bool emissive) {
  q_ = q;
  right_ = right;
  up_ = up;
  auto n = glm::cross(right_, up_);
  norm_ = glm::normalize(n);
  d_ = glm::dot(norm_, q_);
  w_ = n / glm::dot(n, n);
  color_ = color;
  emissive_ = emissive;
}

void Quad::RotateY(float angle) {
  auto R = glm::rotate(mat4x4(1), glm::radians(angle), vec3(0, 1, 0));
  q_ = vec3(R * vec4(q_, 1));
  right_ = vec3(R * vec4(right_, 1));
  up_ = vec3(R * vec4(up_, 1));
  // Recalculate
  auto n = glm::cross(right_, up_);
  norm_ = glm::normalize(n);
  d_ = glm::dot(norm_, q_);
  w_ = n / glm::dot(n, n);
}

void Quad::Translate(vec3 direction) {
  auto T = glm::translate(mat4x4(1), direction);
  q_ = vec3(T * vec4(q_, 1));
  // Recalculate
  auto n = glm::cross(right_, up_);
  norm_ = glm::normalize(n);
  d_ = glm::dot(norm_, q_);
  w_ = n / glm::dot(n, n);
}
