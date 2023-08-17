#pragma once

#include "utils/wgpu_util.h"
#include "objects/triangle.h"
#include "objects/quad.h"
#include "objects/sphere.h"

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
    void LoadObj(const char *file_path, Color3 color);

    void LoadVertices(const char *file_path, std::vector<Vertex> &vertices);

    void InitBindGroupLayout(Device &device);

    void InitBuffers(Device &device);

    Buffer CreateTriangleBuffer(Device &device);

    Buffer CreateQuadBuffer(Device &device);

    void InitBindGroup(Device &device);

public:
    std::vector<Triangle> tris_;
    std::vector<Quad> quads_;
    uint32_t tri_buffer_size_ = 0;
    uint32_t quad_buffer_size_ = 0;
    Buffer tri_buffer_ = nullptr;
    Buffer quad_buffer_ = nullptr;
    Storage storage_ = {};
};
