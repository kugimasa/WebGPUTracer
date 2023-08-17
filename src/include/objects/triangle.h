#pragma once

#include "utils/vec3.h"
#include "objects/vertex.h"

class Triangle {
public:
    Triangle() = default;

    Triangle(Vertex v0, Vertex v1, Vertex v2, Color3 color, bool emissive = false);

public:
    Vertex vertex_[3];
    Vec3 face_norm_, e1_, e2_;
    Color3 color_;
    bool emissive_;
};