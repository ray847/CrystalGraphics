#ifndef SRC_WGPU_RESOURCE_H_
#define SRC_WGPU_RESOURCE_H_

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/camera.h"
#include "CrystalGraphics/error.h"
#include "CrystalGraphics/scene.h"
#include "src/pathtracing/scene_data.h"

namespace crystal::graphics::wgpu {

struct UniformData {
  alignas(16) glm::vec3 position;
  alignas(16) glm::vec3 direction;
  alignas(16) glm::vec2 viewport;
  std::uint32_t _camera_padding[2] = {};
  std::uint32_t iter_count;
  std::uint32_t _padding1[3] = {};
};
static_assert(sizeof(UniformData) == 64);
static_assert(alignof(UniformData) == 16);
static_assert(offsetof(UniformData, iter_count) == 48);

struct Resources {
  ::wgpu::raii::Texture surface_texture;
  ::wgpu::raii::Sampler surface_sampler;
  ::wgpu::raii::Buffer history_buffer;
  ::wgpu::raii::Buffer uniform;
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

std::expected<void, Error> WriteUniform(const Camera& camera,
                                        std::uint32_t iter_count,
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
