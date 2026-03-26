#include "environment.h"

#include <GLFW/glfw3.h>
#include <webgpu/webgpu-raii.hpp>

#include "adapter.h"
#include "device.h"
#include "global.h"
#include "instance.h"
#include "queue.h"
#include "render.h"
#include "surface.h"

namespace crystal::graphics::wgpu {

std::expected<Env, Error> CreateEnv(GLFWwindow* window) {
  /* Instance */
  auto instance = CreateInstance();
  if (!instance) return std::unexpected(instance.error());
  /* Surface */
  auto surface = CreateSurface(*instance, window);
  if (!surface) return std::unexpected(surface.error());
  /* Adapater */
  auto adapter = CreateAdapter(*instance, *surface);
  if (!adapter) return std::unexpected(adapter.error());
  /* Device */
  auto device = CreateDevice(*adapter);
  if (!device) return std::unexpected(device.error());
  /* Configure Surface */
  auto surface_config =
      ConfigSurface(*surface, *adapter, *device);
  if (!surface_config) return std::unexpected(surface_config.error());
  /* Resources */
  auto resources = CreateResources(*surface_config, *device);
  if (!resources) return std::unexpected(resources.error());
  auto surface_texture_view_res =
      CreateSurfaceTextureView(*resources->surface_texture);
  if (!surface_texture_view_res)
    return std::unexpected(surface_texture_view_res.error());
  ::wgpu::raii::TextureView surface_texture_view{ std::move(
      *surface_texture_view_res) };
  /* Compute */
  auto compute_bindgroup_layouts = CreateComputeBindGroupLayouts(*device);
  if (!compute_bindgroup_layouts)
    return std::unexpected(compute_bindgroup_layouts.error());
  auto compute_bindgroup = CreateComputeBindGroups(*surface_texture_view,
                                                   *resources->camera_uniform,
                                                   *resources->bvh_storage,
                                                   *compute_bindgroup_layouts,
                                                   *device);
  if (!compute_bindgroup) return std::unexpected(compute_bindgroup.error());
  auto compute_pipeline =
      CreateComputePipeline(*compute_bindgroup_layouts, *device);
  if (!compute_pipeline) return std::unexpected(compute_pipeline.error());
  /* Render */
  auto render_bindgroup_layout_res = CreateRenderBindGroupLayout(*device);
  if (!render_bindgroup_layout_res)
    return std::unexpected(render_bindgroup_layout_res.error());
  ::wgpu::raii::BindGroupLayout render_bindgroup_layout{ std::move(
      *render_bindgroup_layout_res) };
  auto render_bindgroup = CreateRenderBindGroup(*surface_texture_view,
                                                *resources->surface_sampler,
                                                *render_bindgroup_layout,
                                                *device);
  if (!render_bindgroup) return std::unexpected(render_bindgroup.error());
  auto render_pipeline =
      CreateRenderPipeline(*render_bindgroup_layout, *surface_config, *device);
  if (!render_pipeline) return std::unexpected(render_pipeline.error());
  /* Queue */
  auto queue = CreateQueue(*device);
  if (!queue) return std::unexpected(queue.error());
  return Env{ std::move(*instance),
              std::move(*device),
              std::move(*surface),
              std::move(*resources),
              std::move(*compute_bindgroup_layouts),
              std::move(*compute_bindgroup),
              std::move(*compute_pipeline),
              std::move(*render_bindgroup),
              std::move(*render_pipeline),
              std::move(*queue) };
}

std::expected<void, Error> Env::Sync() {
  if (!instance_)
    return std::unexpected(Error{ "WebGPU environment already terminated." });
  instance_->processEvents();
  if (surface_)
    surface_->present();
  if (auto error = global::error_stack.Pop()) return std::unexpected(*error);
  return {};
}

std::expected<void, Error> Env::Term() {
  if (!instance_)
    return std::unexpected(Error{ "WebGPU environment already terminated." });
  queue_ = ::wgpu::raii::Queue{nullptr};
  surface_->unconfigure();
  surface_ = ::wgpu::raii::Surface{ nullptr };
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  device_ = ::wgpu::raii::Device{ nullptr };
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  instance_ = ::wgpu::raii::Instance{ nullptr };
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return {};
}

Env::operator bool() const {
  return static_cast<bool>(instance_);
}

} // namespace crystal::graphics::wgpu