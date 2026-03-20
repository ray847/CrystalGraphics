#ifndef SRC_WGPU_DEVICE_H_
#define SRC_WGPU_DEVICE_H_

#include <expected>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::Device, Error> CreateDevice(::wgpu::Adapter& adapter);

}  // namespace crystal::graphics::wgpu

#endif