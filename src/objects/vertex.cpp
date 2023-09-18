#include "objects/vertex.h"

Vertex Vertex::Translate(vec3 translation) {
  point_ += translation;
  return *this;
}
