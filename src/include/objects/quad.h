#pragma once

#include "utils/util.h"

class Quad {
public:
    Quad() = default;

    Quad(vec3 center, vec3 right, vec3 up, Color3 color, bool emissive = false);

public:
    vec3 center_;
    vec3 right_;
    vec3 up_;
    vec3 norm_;
    vec3 w_;
    float d_;
    Color3 color_;
    bool emissive_;
};
