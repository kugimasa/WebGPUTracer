#pragma once

#include "GLFW/glfw3.h"
#include "glfw3webgpu.h"
#include "utils/wgpu_util.h"
#include "camera.h"
#include "scene.h"

class Renderer {
public:
    bool OnInit(bool hasWindow);

    bool OnCompute(uint32_t start_frame, uint32_t end_frame);

    bool OnRender(uint32_t frame);

    void OnFrame();

    void OnFinish();

    bool IsRunning();

private:
    bool InitDevice();

    void InitTexture();

    void InitTextureViews();

    void InitSwapChain();

    void InitBindGroupLayout();

    void InitRenderPipeline();

    void InitComputePipeline();

    void InitBuffers();

    void InitBindGroup();

private:
    static const uint32_t WIDTH = 1280;
    static const uint32_t HEIGHT = 960;
    static const uint32_t MAX_FRAME = 600;
    Camera camera_{};
    Scene scene_{};
    bool hasWindow_ = false;
    GLFWwindow *window_ = nullptr;
    Instance instance_ = nullptr;
    Adapter adapter_ = nullptr;
    Device device_ = nullptr;
    Surface surface_ = nullptr;
    Queue queue_ = nullptr;
    SwapChain swap_chain_ = nullptr;
    TextureFormat swap_chain_format_ = TextureFormat::Undefined;
    Texture texture_ = nullptr;
    Extent3D texture_size_ = {WIDTH, HEIGHT, 1};
    TextureView output_texture_view_ = nullptr;
    BindGroupLayout bind_group_layout_ = nullptr;
    PipelineLayout pipeline_layout_ = nullptr;
    RenderPipeline render_pipeline_ = nullptr;
    ComputePipeline compute_pipeline_ = nullptr;
    BindGroup bind_group_ = nullptr;
    uint32_t buffer_size_ = 0;
    Buffer uniform_buffer_ = nullptr;
    Buffer input_buffer_ = nullptr;
    Buffer output_buffer_ = nullptr;
    Buffer map_buffer_ = nullptr;
};