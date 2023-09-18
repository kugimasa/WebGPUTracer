#pragma once

#include "utils/wgpu_util.h"
#include "objects/quad.h"

class Box {
public:
    Box() = default;

    Box(vec3 aabb_min, vec3 aabb_max, Color3 color, float rotation = 0.0f, bool emissive = false);

    void PushQuads(std::vector<Quad> &quads);

private:
    vec3 aabb_min_;
    vec3 aabb_max_;
    vec3 center_;
    double rotation_{};
    Color3 color_;
    bool emissive_{};
    std::vector<Quad> quads_;
};