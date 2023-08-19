#include "objects/vertex.h"

Vertex Vertex::Translate(Vec3 translation) {
  point_ += translation;
  return *this;
}
