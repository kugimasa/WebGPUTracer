#pragma once

#include "utils/util.h"

class Quad {
public:
    Quad() = default;

    Quad(vec3 q, vec3 right, vec3 up, Color3 color, bool emissive = false);

    void RotateY(float angle);

    void Translate(vec3 direction);

public:
    vec3 q_{};
    vec3 right_{};
    vec3 up_{};
    vec3 norm_{};
    vec3 w_{};
    float d_;
    Color3 color_{};
    bool emissive_;
};
