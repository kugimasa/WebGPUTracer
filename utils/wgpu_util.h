/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://eliemichel.github.io/LearnWebGPU
 *
 * MIT License
 * Copyright (c) 2022-2023 Elie Michel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once
#define WEBGPU_CPP_IMPLEMENTATION
#include "webgpu/webgpu.hpp"
using namespace wgpu;
// Dawn and wgpu-native do not agree yet on the lifetime management
// of objects. We align on Dawn convention of calling "release" the
// methods that free memory for objects created with wgpuCreateSomething.
// (The key difference is that Dawn also offers a "reference" function to
// increment a reference counter, and release decreases this counter and
// actually frees memory only when the counter gets to 0)
#ifdef WEBGPU_BACKEND_WGPU
// wgpu-native's non-standard parts are in a different header file:
#include <webgpu/wgpu.h>
#define wgpuInstanceRelease wgpuInstanceDrop
#define wgpuAdapterRelease wgpuAdapterDrop
#define wgpuBindGroupRelease wgpuBindGroupDrop
#define wgpuBindGroupLayoutRelease wgpuBindGroupLayoutDrop
#define wgpuBufferRelease wgpuBufferDrop
#define wgpuCommandBufferRelease wgpuCommandBufferDrop
#define wgpuCommandEncoderRelease wgpuCommandEncoderDrop
#define wgpuRenderPassEncoderRelease wgpuRenderPassEncoderDrop
#define wgpuComputePassEncoderRelease wgpuComputePassEncoderDrop
#define wgpuRenderBundleEncoderRelease wgpuRenderBundleEncoderDrop
#define wgpuComputePipelineRelease wgpuComputePipelineDrop
#define wgpuDeviceRelease wgpuDeviceDrop
#define wgpuPipelineLayoutRelease wgpuPipelineLayoutDrop
#define wgpuQuerySetRelease wgpuQuerySetDrop
#define wgpuRenderBundleRelease wgpuRenderBundleDrop
#define wgpuRenderPipelineRelease wgpuRenderPipelineDrop
#define wgpuSamplerRelease wgpuSamplerDrop
#define wgpuShaderModuleRelease wgpuShaderModuleDrop
#define wgpuSurfaceRelease wgpuSurfaceDrop
#define wgpuSwapChainRelease wgpuSwapChainDrop
#define wgpuTextureRelease wgpuTextureDrop
#define wgpuTextureViewRelease wgpuTextureViewDrop
#define release drop
#endif

///
/// webgpu-release.h copied from "Learn WebGPU for C++" book.
///

#include <cassert>
#include <vector>
#include "print_util.h"

/// \brief Callback function executed upon errors
/// \param type
/// \param message
void inline OnDeviceError(ErrorType type, char const *message) {
  Error(PrintInfoType::WebGPU, "Uncaptured device error: type ", type);
  if (message) {
    Error(PrintInfoType::WebGPU, "message: ", message);
  }
}

/// \brief Callback function to check the queue status
/// \param type
/// \param message
void inline OnQueueWorkDone(QueueWorkDoneStatus status) {
  Print(PrintInfoType::WebGPU, "Queued work finished with status: ", status);
}

/// \brief Show the adapter feature information
/// \param adapter
void inline ShowAdapterFeature(Adapter adapter) {
  std::vector<WGPUFeatureName> features;
  // Call the function a first time with a null return address, just to get the entry count.
  size_t featureCount = adapter.enumerateFeatures(nullptr);
  // Allocate memory (could be a new, or a malloc() if this were a C program)
  features.resize(featureCount);
  // Call the function a second time, with a non-null return address
  wgpuAdapterEnumerateFeatures(adapter, features.data());
  Print(PrintInfoType::WebGPU, "Adapter features: ");
  for (auto f: features) {
    Print(PrintInfoType::WebGPU, " - ", f);
  }
}