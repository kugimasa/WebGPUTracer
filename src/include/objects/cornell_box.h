#pragma once

#include "utils/wgpu_util.h"
#include "quad.h"

class CornellBox {
public:
    CornellBox() = default;

    CornellBox(Point3 center, Vec3 scale);

    void PushToQuads(std::vector<Quad> &quads);

public:
    Point3 center_;
    Vec3 scale_;
    std::vector<Quad> quads_;
};
