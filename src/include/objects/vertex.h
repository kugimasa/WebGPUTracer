#pragma once

#include "utils/vec3.h"

class Vertex {
public:
    Vertex() = default;

    explicit Vertex(Vec3 point, Vec3 normal, float u, float v) :
            point_(point),
            normal_(normal),
            u_(u), v_(v) {}

    Vertex Translate(Vec3 translation);

public:
    Vec3 point_;
    Vec3 normal_;
    float u_, v_;
};
