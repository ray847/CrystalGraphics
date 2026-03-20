#ifndef SRC_WGPU_ADAPTER_H_
#define SRC_WGPU_ADAPTER_H_

#include <expected>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"

namespace crystal::graphics::wgpu {

/* Adapter */
std::expected<::wgpu::Adapter, Error> CreateAdapter(::wgpu::Instance& instance,
                                                    ::wgpu::Surface& surface);

}  // namespace crystal::graphics::wgpu

#endif