#ifndef SRC_WGPU_UTIL_H_
#define SRC_WGPU_UTIL_H_

#include <expected>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"

namespace crystal::graphics::wgpu {

/* Instance */
std::expected<::wgpu::Instance, Error> CreateInstance();

/* Surface */
std::expected<::wgpu::Surface, Error> CreateSurface(::wgpu::Instance& instance,
                                                    GLFWwindow* window);

/* Adapter */
std::expected<::wgpu::Adapter, Error> CreateAdapter(::wgpu::Instance& instance,
                                                    ::wgpu::Surface& surface);

/* Device */
std::expected<::wgpu::Device, Error> CreateDevice(::wgpu::Adapter& adapter);

/* Surface */
std::expected<::wgpu::SurfaceConfiguration, Error> ConfigSurface(
    ::wgpu::Surface& surface,
    ::wgpu::Adapter& adapter,
    ::wgpu::Device& device);
std::expected<::wgpu::TextureView, Error> NextTargetView(
    ::wgpu::Surface& surface);

/* Surface Texture */
std::expected<::wgpu::Texture, Error> CreateSurfaceTexture(
    ::wgpu::Device& device,
    const ::wgpu::SurfaceConfiguration& surface_config);
std::expected<::wgpu::Sampler, Error> CreateSurfaceTextureSampler(
    ::wgpu::Device& device);
std::expected<::wgpu::TextureView, Error> CreateSurfaceTextureView(
    ::wgpu::Texture& surface_texture);

/* Compute */
std::expected<::wgpu::BindGroupLayout, Error> CreateComputeBindGroupLayout(
    ::wgpu::Device& device);
std::expected<::wgpu::BindGroup, Error> CreateComputeBindGroup(
    ::wgpu::TextureView& surface_texture_view,
    ::wgpu::BindGroupLayout& layout,
    ::wgpu::Device& device);
std::expected<::wgpu::ComputePipeline, Error> CreateComputePipeline(
    ::wgpu::BindGroupLayout& bindgroup_layout, ::wgpu::Device& device);
std::expected<void, Error> EncodeComputePass(::wgpu::CommandEncoder& encoder,
                                             ::wgpu::ComputePipeline& pipeline,
                                             ::wgpu::BindGroup& bindgroup);

/* Render */
    std::expected<::wgpu::BindGroupLayout, Error> CreateRenderBindGroupLayout(
        ::wgpu::Device& device);
std::expected<::wgpu::BindGroup, Error> CreateRenderBindGroup(
  ::wgpu::TextureView& surface_texture_view,
  ::wgpu::Sampler& surface_texture_sampler,
  ::wgpu::BindGroupLayout& layout,
  ::wgpu::Device& device
);
std::expected<::wgpu::RenderPipeline, Error> CreateRenderPipeline(
  ::wgpu::BindGroupLayout& bindgroup_layout,
  ::wgpu::SurfaceConfiguration& surface_config,
  ::wgpu::Device& device
);
std::expected<void, Error> EncodeRenderPass(::wgpu::CommandEncoder& encoder,
                                            ::wgpu::TextureView& target_view,
                                            ::wgpu::RenderPipeline& pipeline,
                                            ::wgpu::BindGroup& bindgroup);

/* Queue */
std::expected<::wgpu::Queue, Error> CreateQueue(::wgpu::Device& device);
std::expected<void, Error> Sumbit(::wgpu::Queue& queue,
                                  ::wgpu::CommandBuffer& cmd_buffer);

/* Command */
std::expected<::wgpu::CommandEncoder, Error> CreateCommandEncoder(
    ::wgpu::Device& device);
std::expected<::wgpu::CommandBuffer, Error> CreateCommandBuffer(
    ::wgpu::CommandEncoder& encoder);

} // namespace crystal::graphics

#endif