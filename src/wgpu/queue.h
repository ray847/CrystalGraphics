#ifndef SRC_WGPU_QUEUE_H_
#define SRC_WGPU_QUEUE_H_

#include <expected>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::Queue, Error> CreateQueue(::wgpu::Device& device);

std::expected<void, Error> Sumbit(::wgpu::Queue& queue,
                                  ::wgpu::CommandBuffer& cmd_buffer);

} // namespace crystal::graphics

#endif