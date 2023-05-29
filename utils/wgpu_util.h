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
#include <webgpu/webgpu.h>

// Dawn and wgpu-native do not agree yet on the lifetime management
// of objects. We align on Dawn convention of calling "release" the
// methods that free memory for objects created with wgpuCreateSomething.
// (The key difference is that Dawn also offers a "reference" function to
// increment a reference counter, and release decreases this counter and
// actually frees memory only when the counter gets to 0)
#ifdef WEBGPU_BACKEND_WGPU
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
#endif

///
/// webgpu-release.h copied from "Learn WebGPU for C++" book.
///

#include <cassert>
#include <vector>
#include "print_util.h"

/// \brief Utility function to get a WebGPU adapter
/// \param instance The WebGPU instance equivalent to `navigator.gpu`
/// \param options
/// \return
WGPUAdapter inline RequestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const *options) {
  // Local information
  struct UserData {
    WGPUAdapter adapter = nullptr;
    bool request_ended = false;
  };

  UserData user_data;

  // Callback function for adapter request
  auto OnAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const *message, void *p_user_data) {
    UserData &user_data = *reinterpret_cast<UserData *>(p_user_data);
    if (status == WGPURequestAdapterStatus_Success) {
      user_data.adapter = adapter;
    } else {
      Error(PrintInfoType::WebGPU, "Could not get WebGPU adapter: ", message);
    }
    user_data.request_ended = true;
  };

  // Call to the WebGPU request adapter procedure
  wgpuInstanceRequestAdapter(
      instance,
      options,
      OnAdapterRequestEnded,
      (void *) &user_data
  );

  assert(user_data.request_ended);

  return user_data.adapter;
}

/// \brief Utility function to get a WebGPU device
/// \param adapter
/// \param descriptor
/// \return
WGPUDevice inline RequestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const *descriptor) {
  struct UserData {
    WGPUDevice device = nullptr;
    bool request_ended = false;
  };

  UserData user_data;

  // Callback function for device request
  auto OnDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const *message, void *p_user_data) {
    UserData &user_data = *reinterpret_cast<UserData *>(p_user_data);
    if (status == WGPURequestDeviceStatus_Success) {
      user_data.device = device;
    } else {
      Error(PrintInfoType::WebGPU, "Could not get WebGPU device: ", message);
    }
    user_data.request_ended = true;
  };

  wgpuAdapterRequestDevice(
      adapter,
      descriptor,
      OnDeviceRequestEnded,
      (void *) &user_data
  );

  assert(user_data.request_ended);

  return user_data.device;
}

/// \brief Callback function executed upon errors
/// \param type
/// \param message
void inline OnDeviceError(WGPUErrorType type, char const *message, void *) {
  Error(PrintInfoType::WebGPU, "Uncaptured device error: type ", type);
  if (message) {
    Error(PrintInfoType::WebGPU, "message: ", message);
  }
}

/// \brief Show the adapter feature information
/// \param adapter
void inline ShowAdapterFeature(WGPUAdapter adapter) {
  std::vector<WGPUFeatureName> features;

  // Call the function a first time with a null return address, just to get the entry count.
  size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);
  // Allocate memory (could be a new, or a malloc() if this were a C program)
  features.resize(featureCount);

  // Call the function a second time, with a non-null return address
  wgpuAdapterEnumerateFeatures(adapter, features.data());
  Print(PrintInfoType::WebGPU, "Adapter features: ");
  for (auto f: features) {
    Print(PrintInfoType::WebGPU, " - ", f);
  }
}