#pragma once

#include "utils/wgpu_util.h"
#include "quad.h"

class CornellBox {
public:
    CornellBox() = default;

    CornellBox(Device &device);

public:
    uint32_t vertex_count_;
    uint32_t index_count_;
    Buffer vertices_ = nullptr;
    Buffer indices_ = nullptr;
    Buffer quad_buffer_ = nullptr;
    VertexBufferLayout vertex_buffer_layout_;
    std::vector<Quad> quads_;
};
