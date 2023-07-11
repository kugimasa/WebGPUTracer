#pragma once

#include "utils/wgpu_util.h"

class Camera {
public:
    Camera() = default;

    explicit Camera(Device device);

    ~Camera();

    struct Uniforms {
        BindGroupLayout bind_group_layout_;
        BindGroup bind_group_;

        Uniforms() : bind_group_layout_(nullptr), bind_group_(nullptr) {};
    };

    Uniforms GetUniforms() { return uniforms_; }

    void Release();

private:
    void InitBindGroupLayout();

    void InitBuffers();

    void InitBindGroup();

private:

    Device device_ = nullptr;
    // TODO: Scene
    uint32_t buffer_size_ = 0;
    Buffer uniform_buffer_ = nullptr;
    Uniforms uniforms_ = {};
};
