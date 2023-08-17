#include "objects/triangle.h"

Triangle::Triangle(Vertex v0, Vertex v1, Vertex v2, Color3 color, bool emissive) {
  // 頂点
  vertex_[0] = v0;
  vertex_[1] = v1;
  vertex_[2] = v2;
  // 面法線
  e1_ = vertex_[1].point_ - vertex_[0].point_;
  e2_ = vertex_[2].point_ - vertex_[0].point_;
  face_norm_ = Unit(Cross(e1_, e2_));
  // カラー
  color_ = color;
  // エミッシブ
  emissive_ = emissive;
}