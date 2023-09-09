#include "objects/cornell_box.h"

CornellBox::CornellBox() {
  auto red = Color3(.65, .05, .05);
  auto white = Color3(.73, .73, .73);
  auto green = Color3(.12, .45, .15);
  auto light = Color3(15, 15, 15);

  // Walls
  quads_.emplace_back(Point3(555, 0, 0), Vec3(0, 0, 555), Vec3(0, 555, 0), green);
  quads_.emplace_back(Point3(0, 0, 555), Vec3(0, 0, -555), Vec3(0, 555, 0), red);
  quads_.emplace_back(Point3(0, 555, 0), Vec3(555, 0, 0), Vec3(0, 0, 555), white);
  quads_.emplace_back(Point3(0, 0, 555), Vec3(555, 0, 0), Vec3(0, 0, -555), white);
  quads_.emplace_back(Point3(555, 0, 555), Vec3(-555, 0, 0), Vec3(0, 555, 0), white);

  // Light
  quads_.emplace_back(Point3(213, 554, 227), Vec3(130, 0, 0), Vec3(0, 0, 105), light, true);
}

void CornellBox::PushToQuads(std::vector<Quad> &quads) {
  quads.insert(quads.end(), quads_.begin(), quads_.end());
}
