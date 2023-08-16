#pragma once

#include "utils/wgpu_util.h"
#include "quad.h"

class CornellBox {
public:
    CornellBox() = default;

    CornellBox(Device &device);

public:
    Buffer quad_buffer_ = nullptr;
    std::vector<Quad> quads_;
};
