#include "objects/box.h"

Box::Box(Vec3 center, Vec3 scale, float rotation, Color3 color, bool emissive, bool face_out) {
  center_ = center;
  scale_ = scale;
  rotation_ = rotation;
  color_ = color;
  emissive_ = emissive;
  face_out_ = face_out;
  const Vec3 x = Vec3(cos(rotation) * (scale.X() / 2), 0, sin(rotation) * (scale.Z() / 2));
  const Vec3 y = Vec3(0, scale.Y() / 2, 0);
  const Vec3 z = Vec3(sin(rotation) * (scale.X() / 2), 0, -cos(rotation) * (scale.Z() / 2));
  Quad pos_x = Quad(center + x, FlipVec3(-z), y, color, emissive);
  Quad pos_y = Quad(center + y, FlipVec3(x), -z, color, emissive);
  Quad pos_z = Quad(center + z, FlipVec3(x), y, color, emissive);
  Quad neg_x = Quad(center - x, FlipVec3(z), y, color, emissive);
  Quad neg_y = Quad(center - y, FlipVec3(x), z, color, emissive);
  Quad neg_z = Quad(center - z, FlipVec3(-x), y, color, emissive);
  quads_.push_back(pos_x);
  quads_.push_back(pos_y);
  quads_.push_back(pos_z);
  quads_.push_back(neg_x);
  quads_.push_back(neg_y);
  quads_.push_back(neg_z);
}

void Box::PushQuads(std::vector<Quad> &quads) {
  for (int i = 0; i < (int) quads_.size(); ++i) {
    quads.push_back(quads_[i]);
  }
}

Vec3 Box::FlipVec3(Vec3 v) const {
  return face_out_ ? -v : v;
}

