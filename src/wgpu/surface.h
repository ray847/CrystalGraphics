#ifndef SRC_WGPU_SURFACE_H_
#define SRC_WGPU_SURFACE_H_

#include <expected>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::Surface, Error> CreateSurface(::wgpu::Instance& instance,
                                                    GLFWwindow* window);

std::expected<::wgpu::SurfaceConfiguration, Error> ConfigSurface(
    ::wgpu::Surface& surface,
    ::wgpu::Adapter& adapter,
    ::wgpu::Device& device);

std::expected<::wgpu::TextureView, Error> NextTargetView(
    ::wgpu::Surface& surface);

}

#endif