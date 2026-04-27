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
#include "CrystalGraphics/scene.h"
#include "src/pathtracing/scene_data.h"

namespace crystal::graphics::wgpu {

struct Resources {
  ::wgpu::raii::Texture surface_texture;
  ::wgpu::raii::Sampler surface_sampler;
  ::wgpu::raii::Buffer camera_uniform;
  ::wgpu::raii::Buffer tlas_storage;
  std::size_t inst_offset;
  ::wgpu::raii::Buffer blas_storage;
  std::size_t idx_offset, vert_offset, mat_offset;
};

std::expected<Resources, Error> CreateResources(
    const ::wgpu::SurfaceConfiguration& surface_config,
    std::size_t min_offset_alignment,
    ::wgpu::Device& device);

std::expected<::wgpu::TextureView, Error> CreateSurfaceTextureView(
    ::wgpu::Texture& surface_texture);

std::expected<void, Error> WriteCameraUniform(const Camera& camera,
                                              Resources& resources,
                                              ::wgpu::Queue& queue);

std::expected<bool, Error> WriteScene(const SceneData& scene_data,
                                      Resources& resources,
                                      std::size_t min_offset_alignment,
                                      ::wgpu::Queue& queue,
                                      ::wgpu::Device& device);

}  // namespace crystal::graphics::wgpu

#endif
