#pragma once

#include "utils/util.h"

class Vertex {
public:
    Vertex() = default;

    explicit Vertex(vec3 point, vec3 normal, float u, float v) :
            point_(point),
            normal_(normal),
            u_(u), v_(v) {}

    Vertex Translate(vec3 translation);

public:
    vec3 point_;
    vec3 normal_;
    float u_, v_;
};
