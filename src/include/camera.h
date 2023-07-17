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

    void Update(Queue &queue, Point3 origin, Point3 target, float aspect, float time);

private:
    void InitBindGroupLayout(Device &device);

    void InitBuffers(Device &device);

    void InitBindGroup(Device &device);


private:
    struct Ray {
        Point3 origin;
        Point3 target;
        float aspect;
        float time;
        float rand;

        Ray(Vec3 origin, Vec3 target, float aspect, float time, float rand) :
                origin(origin), target(target), aspect(aspect), time(time), rand(rand) {}
    };


private:
    uint32_t buffer_size_ = 0;
    Buffer uniform_buffer_ = nullptr;
    Uniforms uniforms_ = {};
    Point3 origin_{0.0, 0.0, 0.0};
    Point3 target_{0.0, 0.0, 0.0};
};
