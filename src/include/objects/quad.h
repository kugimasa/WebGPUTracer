#pragma once

#include "utils/vec3.h"

class Quad {
public:
    Quad() = default;

    explicit Quad(Vec3 center, Vec3 right, Vec3 up, Color3 color, bool emissive = false) :
            center_(center),
            right_(right),
            up_(up),
            color_(color),
            emissive_(emissive) {}

public:
    Vec3 center_;
    Vec3 right_;
    Vec3 up_;
    Color3 color_;
    bool emissive_;
};
