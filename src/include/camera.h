#pragma once

#include "utils/wgpu_util.h"
#include "utils/util.h"

class Camera {
public:
    Camera() = default;

    explicit Camera(Device &device, uint32_t spp);

    struct Uniforms {
        BindGroupLayout bind_group_layout_;
        BindGroup bind_group_;

        Uniforms() : bind_group_layout_(nullptr), bind_group_(nullptr) {};
    };

    struct CameraParam {
        Point3 origin;
        float dummy{};
        Point3 target;
        float dummy1{};
        float aspect;
        float fovy;
        uint32_t spp;
        uint32_t seed;

        CameraParam(vec3 origin, vec3 target, float aspect, float fovy, uint32_t spp, uint32_t seed) :
                origin(origin), target(target), aspect(aspect), fovy(fovy), spp(spp), seed(seed) {}
    };

    Uniforms GetUniforms() { return uniforms_; }

    void Release();

    void Update(Queue &queue, float t, float aspect);

private:
    void InitBindGroupLayout(Device &device);

    void InitBuffers(Device &device);

    void InitBindGroup(Device &device);

private:
    uint32_t spp_{1};
    Buffer uniform_buffer_ = nullptr;
    Uniforms uniforms_ = {};
};
