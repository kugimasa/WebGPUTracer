#pragma once

#include "utils/vec3.h"

class Sphere {
public:
    Sphere() = default;

    explicit Sphere(Point3 center, float radius, Color3 color, bool emissive = false) :
            center_(center),
            radius_(radius),
            color_(color),
            emissive_(emissive) {}

public:
    Point3 center_;
    float radius_;
    Color3 color_;
    bool emissive_;
};