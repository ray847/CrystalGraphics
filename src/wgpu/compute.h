#ifndef SRC_WGPU_COMPUTE_H_
#define SRC_WGPU_COMPUTE_H_

#include <expected>
#include <array>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"

namespace crystal::graphics::wgpu {

struct ComputeBindGroupLayouts {
  std::array<::wgpu::BindGroupLayout, 2> layouts;
  ComputeBindGroupLayouts(::wgpu::BindGroupLayout layout0,
                   ::wgpu::BindGroupLayout layout1) :
      layouts{ layout0, layout1 } {
  }
  ~ComputeBindGroupLayouts() {
    for (auto& layout : layouts) {
      if (layout) layout.release();
    }
  }
  ComputeBindGroupLayouts(const ComputeBindGroupLayouts&) = delete;
  ComputeBindGroupLayouts& operator=(const ComputeBindGroupLayouts&) = delete;
  ComputeBindGroupLayouts(ComputeBindGroupLayouts&&) = default;
  ComputeBindGroupLayouts& operator=(ComputeBindGroupLayouts&&) = default;

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
  std::array<::wgpu::BindGroup, 2> bindgroups;
  ComputeBindGroups(::wgpu::BindGroup group0, ::wgpu::BindGroup group1) :
      bindgroups{ group0, group1 } {
  }
  ~ComputeBindGroups() {
    for (auto& bindgroup : bindgroups)
      if (bindgroup) bindgroup.release();
  }
  ComputeBindGroups(const ComputeBindGroups&) = delete;
  ComputeBindGroups& operator=(const ComputeBindGroups&) = delete;
  ComputeBindGroups(ComputeBindGroups&&) = default;
  ComputeBindGroups& operator=(ComputeBindGroups&&) = default;

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
    ::wgpu::TextureView& surface_texture_view,
    ::wgpu::Buffer& camera_uniform,
    ComputeBindGroupLayouts& layout,
    ::wgpu::Device& device);

std::expected<::wgpu::ComputePipeline, Error> CreateComputePipeline(
    ComputeBindGroupLayouts& bindgroup_layout, ::wgpu::Device& device);

std::expected<void, Error> EncodeComputePass(::wgpu::CommandEncoder& encoder,
                                             ::wgpu::ComputePipeline& pipeline,
                                             ComputeBindGroups& bindgroup);

}

#endif