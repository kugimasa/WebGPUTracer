#pragma once

#include "utils/wgpu_util.h"
#include "utils/util.h"

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

    void Update(Queue &queue, float aspect);

private:
    void InitBindGroupLayout(Device &device);

    void InitBuffers(Device &device);

    void InitBindGroup(Device &device);

private:
    struct CameraUniforms {
        mat4x4 mvp;
        mat4x4 inv_mvp;
        std::array<float, 4> seed;

        CameraUniforms(mat4x4 mvp, mat4x4 inv_mvp, std::array<float, 4> seed) :
                mvp(mvp), inv_mvp(inv_mvp), seed(seed) {}
    };

    uint32_t buffer_size_ = 0;
    Buffer uniform_buffer_ = nullptr;
    Uniforms uniforms_ = {};
};
