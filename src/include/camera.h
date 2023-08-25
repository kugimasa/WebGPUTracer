#pragma once

#include "utils/wgpu_util.h"
#include "utils/util.h"
#include "utils/vec3.h"

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

    void Update(Queue &queue, float t, float aspect);

private:
    void InitBindGroupLayout(Device &device);

    void InitBuffers(Device &device);

    void InitBindGroup(Device &device);


private:
    struct CameraParam {
        Point3 origin;
        float dummy;
        Point3 target;
        float dummy1;
        float aspect;
        float fovy;
        float time;
        uint32_t seed;

        CameraParam(Vec3 origin, Vec3 target, float aspect, float fovy, float time, uint32_t seed) :
                origin(origin), target(target), aspect(aspect), fovy(fovy), time(time), seed(seed) {}
    };


private:
    Buffer uniform_buffer_ = nullptr;
    Uniforms uniforms_ = {};
};
