#include "objects/cornell_box.h"
#include "utils/color_util.h"

CornellBox::CornellBox() {
  // Walls
  quads_.emplace_back(Point3(555, 0, 0), vec3(0, 0, 555), vec3(0, 555, 0), COL_GREEN);
  quads_.emplace_back(Point3(0, 0, 555), vec3(0, 0, -555), vec3(0, 555, 0), COL_RED);
  quads_.emplace_back(Point3(0, 555, 0), vec3(555, 0, 0), vec3(0, 0, 555), COL_WHITE);
  quads_.emplace_back(Point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 0, -555), COL_WHITE);
  quads_.emplace_back(Point3(555, 0, 555), vec3(-555, 0, 0), vec3(0, 555, 0), COL_WHITE);

  // Light
  quads_.emplace_back(Point3(213, 554, 227), vec3(130, 0, 0), vec3(0, 0, 105), COL_LIGHT, true);
}

void CornellBox::PushToQuads(std::vector<Quad> &quads) {
  quads.insert(quads.end(), quads_.begin(), quads_.end());
}
