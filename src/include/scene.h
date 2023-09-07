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

    void Release();

private:
    void LoadObj(const char *file_path, Color3 color, Vec3 translation = ZERO_Vec3, bool emissive = false);

    void LoadVertices(const char *file_path, std::vector<Vertex> &vertices);

    void InitBindGroupLayout(Device &device);

    void InitBuffers(Device &device);

    Buffer CreateTriangleBuffer(Device &device);

    Buffer CreateQuadBuffer(Device &device);

    Buffer CreateSphereBuffer(Device &device, size_t num, WGPUBufferUsageFlags usage_flags, bool mapped_at_creation);

    void InitBindGroup(Device &device);

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
    Objects objects_ = {};
};
