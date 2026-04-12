#ifndef SRC_WGPU_RESOURCE_H_
#define SRC_WGPU_RESOURCE_H_

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#include <expected>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/camera.h"
#include "CrystalGraphics/error.h"
#include "src/pathtracing/bvh.h"

namespace crystal::graphics::wgpu {

struct Resources {
  ::wgpu::raii::Texture surface_texture;
  ::wgpu::raii::Sampler surface_sampler;
  ::wgpu::raii::Buffer camera_uniform;
  ::wgpu::raii::Buffer tlas_storage;
  ::wgpu::raii::Buffer blas_storage;
};

std::expected<Resources, Error> CreateResources(
    const ::wgpu::SurfaceConfiguration& surface_config, ::wgpu::Device& device);

std::expected<::wgpu::TextureView, Error> CreateSurfaceTextureView(
    ::wgpu::Texture& surface_texture);

std::expected<void, Error> WriteCameraUniform(const Camera& camera,
                                              Resources& resources,
                                              ::wgpu::Queue& queue);

std::expected<bool, Error> WriteBVHStorage(const BVH& bvh,
                                           Resources& resources,
                                           ::wgpu::Queue& queue,
                                           ::wgpu::Device& device);

}  // namespace crystal::graphics::wgpu

#endif