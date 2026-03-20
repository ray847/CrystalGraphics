#ifndef SRC_WGPU_RENDER_H_
#define SRC_WGPU_RENDER_H_

#include <expected>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::BindGroupLayout, Error> CreateRenderBindGroupLayout(
    ::wgpu::Device& device);

std::expected<::wgpu::BindGroup, Error> CreateRenderBindGroup(
    ::wgpu::TextureView& surface_texture_view,
    ::wgpu::Sampler& surface_texture_sampler,
    ::wgpu::BindGroupLayout& layout,
    ::wgpu::Device& device);

std::expected<::wgpu::RenderPipeline, Error> CreateRenderPipeline(
    ::wgpu::BindGroupLayout& bindgroup_layout,
    ::wgpu::SurfaceConfiguration& surface_config,
    ::wgpu::Device& device);

std::expected<void, Error> EncodeRenderPass(::wgpu::CommandEncoder& encoder,
                                            ::wgpu::TextureView& target_view,
                                            ::wgpu::RenderPipeline& pipeline,
                                            ::wgpu::BindGroup& bindgroup);

}  // namespace crystal::graphics::wgpu

#endif