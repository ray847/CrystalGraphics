#ifndef SRC_WGPU_RESOURCE_H_
#define SRC_WGPU_RESOURCE_H_

#include <expected>
#include <array>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"
#include "CrystalGraphics/camera.h"

namespace crystal::graphics::wgpu {

struct Resources {
  ::wgpu::raii::Texture surface_texture;
  ::wgpu::raii::Sampler surface_sampler;
  ::wgpu::raii::Buffer camera_uniform;
};

std::expected<Resources, Error> CreateResources(
    const ::wgpu::SurfaceConfiguration& surface_config, ::wgpu::Device& device);

std::expected<::wgpu::TextureView, Error> CreateSurfaceTextureView(
    ::wgpu::Texture& surface_texture);

std::expected<void, Error> WriteCameraUniform(const Camera& camera,
                                              Resources& resources,
                                              ::wgpu::Queue& queue);
}

#endif