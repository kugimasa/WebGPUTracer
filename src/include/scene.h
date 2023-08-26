#pragma once

#include "utils/wgpu_util.h"
#include "objects/triangle.h"
#include "objects/quad.h"
#include "objects/sphere.h"

class Scene {
public:
    Scene() = default;

    explicit Scene(Device &device);

    struct Objects {
        BindGroupLayout bind_group_layout_;
        BindGroup bind_group_;

        Objects() : bind_group_layout_(nullptr), bind_group_(nullptr) {};
    };

    struct SphereLights {
        Sphere l1_;
        Sphere l2_;
        Sphere l3_;
        Sphere l4_;
        Sphere l5_;
        Sphere l6_;
        Sphere l7_;
        Sphere l8_;

        SphereLights(Sphere l1, Sphere l2, Sphere l3, Sphere l4, Sphere l5, Sphere l6, Sphere l7, Sphere l8)
                : l1_(l1), l2_(l2), l3_(l3), l4_(l4), l5_(l5), l6_(l6), l7_(l7), l8_(l8) {};
    };

    void Release();

    void UpdateSphereLights(Queue &queue, float t);

private:
    void LoadObj(const char *file_path, Color3 color, Vec3 translation = ZERO_Vec3, bool emissive = false);

    void LoadVertices(const char *file_path, std::vector<Vertex> &vertices);

    void InitBindGroupLayout(Device &device);

    void InitBuffers(Device &device);

    Buffer CreateTriangleBuffer(Device &device);

    Buffer CreateQuadBuffer(Device &device);

    Buffer CreateSphereBuffer(Device &device, uint32_t num, WGPUBufferUsageFlags usage_flags, bool mapped_at_creation);

    void InitBindGroup(Device &device);

    size_t inline GetSphereLightsNum() {
      return sizeof(SphereLights) / sizeof(Sphere);
    };

public:
    std::vector<Triangle> tris_;
    std::vector<Quad> quads_;
    std::vector<Sphere> spheres_;
    uint32_t tri_stride_ = 20 * 4;
    uint32_t quad_stride_ = 24 * 4;
    uint32_t sphere_stride_ = 8 * 4;
    Buffer tri_buffer_ = nullptr;
    Buffer quad_buffer_ = nullptr;
    Buffer sphere_buffer_ = nullptr;
    Buffer sphere_light_buffer_ = nullptr;
    Objects objects_ = {};
};
