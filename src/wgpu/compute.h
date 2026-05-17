#ifndef SRC_WGPU_COMPUTE_H_
#define SRC_WGPU_COMPUTE_H_

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#include <array>
#include <expected>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"

namespace crystal::graphics::wgpu {

struct ComputeBindGroupLayouts {
  /**
   * Compute BindGroup Layout.
   *
   * 0. Target Texture
   * 1. Camera
   * 2. Scene Storage, Material Textures
   */
  std::array<::wgpu::BindGroupLayout, 3> layouts;
  ComputeBindGroupLayouts(::wgpu::BindGroupLayout layout0,
                          ::wgpu::BindGroupLayout layout1,
                          ::wgpu::BindGroupLayout layout2) :
      layouts{ layout0, layout1, layout2 } {
  }
  ~ComputeBindGroupLayouts() {
    for (auto& layout : layouts) {
      if (layout) layout.release();
    }
  }
  ComputeBindGroupLayouts(const ComputeBindGroupLayouts&) = delete;
  ComputeBindGroupLayouts& operator=(const ComputeBindGroupLayouts&) = delete;
  ComputeBindGroupLayouts(ComputeBindGroupLayouts&& other) noexcept :
      layouts{ other.layouts } {
    other.layouts = { nullptr, nullptr, nullptr };
  }
  ComputeBindGroupLayouts& operator=(ComputeBindGroupLayouts&& other) noexcept {
    if (this != &other) {
      for (auto& layout : layouts) {
        if (layout) layout.release();
      }
      layouts = other.layouts;
      other.layouts = { nullptr, nullptr, nullptr };
    }
    return *this;
  }

  auto& operator[](std::size_t idx) {
    return layouts[idx];
  }
  auto& operator*() {
    return layouts;
  }
  WGPUBindGroupLayout* data() {
    return reinterpret_cast<WGPUBindGroupLayout*>(layouts.data());
  }
  size_t size() const {
    return layouts.size();
  }
};

struct ComputeBindGroups {
  std::array<::wgpu::BindGroup, 3> bindgroups;
  ::wgpu::TextureView material_texture_array_view;
  ComputeBindGroups(::wgpu::BindGroup group0,
                    ::wgpu::BindGroup group1,
                    ::wgpu::BindGroup group2,
                    ::wgpu::TextureView material_texture_array_view) :
      bindgroups{ group0, group1, group2 },
      material_texture_array_view{ material_texture_array_view } {
  }
  ~ComputeBindGroups() {
    for (auto& bindgroup : bindgroups)
      if (bindgroup) bindgroup.release();
    if (material_texture_array_view) material_texture_array_view.release();
  }
  ComputeBindGroups(const ComputeBindGroups&) = delete;
  ComputeBindGroups& operator=(const ComputeBindGroups&) = delete;
  ComputeBindGroups(ComputeBindGroups&& other) noexcept :
      bindgroups{ other.bindgroups },
      material_texture_array_view{ other.material_texture_array_view } {
    other.bindgroups = { nullptr, nullptr, nullptr };
    other.material_texture_array_view = nullptr;
  }
  ComputeBindGroups& operator=(ComputeBindGroups&& other) noexcept {
    if (this != &other) {
      for (auto& layout : bindgroups) {
        if (layout) layout.release();
      }
      if (material_texture_array_view) material_texture_array_view.release();
      bindgroups = other.bindgroups;
      material_texture_array_view = other.material_texture_array_view;
      other.bindgroups = { nullptr, nullptr, nullptr };
      other.material_texture_array_view = nullptr;
    }
    return *this;
  }

  auto& operator[](std::size_t idx) {
    return bindgroups[idx];
  }
  auto& operator*() {
    return bindgroups;
  }
  WGPUBindGroup* data() {
    return reinterpret_cast<WGPUBindGroup*>(bindgroups.data());
  }
  size_t size() const {
    return bindgroups.size();
  }
};

std::expected<ComputeBindGroupLayouts, Error> CreateComputeBindGroupLayouts(
    ::wgpu::Device& device);

std::expected<ComputeBindGroups, Error> CreateComputeBindGroups(
    /* Group 0 */
    ::wgpu::TextureView& surface_texture_view,
    /* Group 1 */
    ::wgpu::Buffer& camera_uniform,
    /* Group 2 */
    ::wgpu::Buffer& tlas_inst_storage,
    std::size_t insts_offset,
    ::wgpu::Buffer& blas_idx_vert_storage,
    std::size_t idx_offset,
    std::size_t vert_offset,
    std::size_t mat_offset,
    ::wgpu::Texture& material_texture_array,
    ::wgpu::Sampler& material_texture_sampler,
    ComputeBindGroupLayouts& layouts,
    ::wgpu::Device& device);

std::expected<void, Error> UpdateComputeBindGroup2(
    ComputeBindGroups& bindgroups,
    ::wgpu::Buffer& tlas_storage,
    std::size_t insts_offset,
    ::wgpu::Buffer& blas_idx_vert_storage,
    std::size_t idx_offset,
    std::size_t vert_offset,
    std::size_t mat_offset,
    ::wgpu::Texture& material_texture_array,
    ::wgpu::Sampler& material_texture_sampler,
    ComputeBindGroupLayouts& layouts,
    ::wgpu::Device& device);

std::expected<::wgpu::ComputePipeline, Error> CreateComputePipeline(
    ComputeBindGroupLayouts& bindgroup_layout, ::wgpu::Device& device);

std::expected<void, Error> EncodeComputePass(::wgpu::CommandEncoder& encoder,
                                             ::wgpu::ComputePipeline& pipeline,
                                             ComputeBindGroups& bindgroup);

}  // namespace crystal::graphics::wgpu

#endif
