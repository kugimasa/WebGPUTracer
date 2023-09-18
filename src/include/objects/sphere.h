#pragma once

#include "utils/util.h"

class Sphere {
public:
    Sphere() = default;

    explicit Sphere(Point3 center, float radius, Color3 color, float emissive = 0.0f) :
            center_(center),
            radius_(radius),
            color_(color),
            emissive_(emissive) {}

public:
    Point3 center_;
    float radius_;
    Color3 color_;
    float emissive_;
};