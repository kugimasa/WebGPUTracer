#pragma once

#include "utils/wgpu_util.h"
#include "triangle.h"
#include "quad.h"

class Scene {
public:
    Scene() = default;

    Scene(Device &device);

    struct Storage {
        BindGroupLayout bind_group_layout_;
        BindGroup bind_group_;

        Storage() : bind_group_layout_(nullptr), bind_group_(nullptr) {};
    };

    void Release();

private:
    void InitBindGroupLayout(Device &device);

    void InitBuffers(Device &device);

    void InitBindGroup(Device &device);

public:
    std::vector<Triangle> tris_;
    std::vector<Quad> quads_;
    uint32_t scene_buffer_size_ = 0;
    Buffer scene_buffer_ = nullptr;
    Storage storage_ = {};
};
