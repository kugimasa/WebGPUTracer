#pragma once

#include "utils/wgpu_util.h"
#include "objects/quad.h"

class Box {
public:
    Box() = default;

    Box(Vec3 aabb_min, Vec3 aabb_max, Color3 color, float rotation = 0.0f, bool emissive = false);

    void PushQuads(std::vector<Quad> &quads);

private:
    Vec3 aabb_min_;
    Vec3 aabb_max_;
    Vec3 center_;
    double rotation_{};
    Color3 color_;
    bool emissive_{};
    std::vector<Quad> quads_;
};