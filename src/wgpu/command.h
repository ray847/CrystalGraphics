#ifndef SRC_WGPU_COMMAND_H_
#define SRC_WGPU_COMMAND_H_

#include <expected>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::CommandEncoder, Error> CreateCommandEncoder(
    ::wgpu::Device& device);

std::expected<::wgpu::CommandBuffer, Error> CreateCommandBuffer(
    ::wgpu::CommandEncoder& encoder);

} // namespace crystal::graphics

#endif