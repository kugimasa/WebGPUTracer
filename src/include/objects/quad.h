#pragma once

#include "utils/vec3.h"

class Quad {
public:
    Quad() = default;

    Quad(Vec3 center, Vec3 right, Vec3 up, Color3 color, bool emissive = false);

public:
    Vec3 center_;
    Vec3 right_;
    Vec3 up_;
    Vec3 norm_;
    Vec3 w_;
    float d_;
    Color3 color_;
    bool emissive_;
};
