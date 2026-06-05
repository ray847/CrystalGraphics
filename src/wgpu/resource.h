#ifndef SRC_WGPU_RESOURCE_H_
#define SRC_WGPU_RESOURCE_H_

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#include <expected>
#include <cstdint>
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
  ::wgpu::raii::Buffer scene_storage;
  std::size_t idx_offset, vert_offset, mat_offset, emissive_offset,
      alias_offset;
  ::wgpu::raii::Texture material_texture_array;
  ::wgpu::raii::Sampler material_texture_sampler;
  ::wgpu::raii::Texture environment_texture;
  ::wgpu::raii::Sampler environment_texture_sampler;
  uint32_t material_texture_width;
  uint32_t material_texture_height;
  uint32_t material_texture_layers;
  uint32_t environment_texture_width;
  uint32_t environment_texture_height;
};

struct SceneWriteResult {
  bool storage_changed;
  bool material_textures_changed;
  bool environment_texture_changed;
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

std::expected<SceneWriteResult, Error> WriteScene(
    const SceneData& scene_data,
    Resources& resources,
    std::size_t min_offset_alignment,
    ::wgpu::Queue& queue,
    ::wgpu::Device& device);

}  // namespace crystal::graphics::wgpu

#endif
