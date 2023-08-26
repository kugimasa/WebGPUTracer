#pragma once

#include "utils/wgpu_util.h"
#include "objects/quad.h"

class Box {
public:
    Box() = default;

    Box(Vec3 center, Vec3 scale, float rotation, Color3 color, bool emissive = false, bool face_out_ = true);

    void PushQuads(std::vector<Quad> &quads);

private:
    [[nodiscard]] Vec3 FlipVec3(Vec3 v) const;

private:
    Vec3 center_;
    Vec3 scale_;
    double rotation_{};
    Color3 color_;
    bool emissive_{};
    bool face_out_{};
    std::vector<Quad> quads_;
};