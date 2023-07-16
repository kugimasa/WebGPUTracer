#pragma once

#include "utils/wgpu_util.h"

class Camera {
public:
    Camera() = default;

    explicit Camera(Device &device);

    struct Uniforms {
        BindGroupLayout bind_group_layout_;
        BindGroup bind_group_;

        Uniforms() : bind_group_layout_(nullptr), bind_group_(nullptr) {};
    };

    Uniforms GetUniforms() { return uniforms_; }

    void Release();

private:
    void InitBindGroupLayout(Device &device);

    void InitBuffers(Device &device);

    void InitBindGroup(Device &device);

private:
    uint32_t buffer_size_ = 0;
    Buffer uniform_buffer_ = nullptr;
    Uniforms uniforms_ = {};
};
