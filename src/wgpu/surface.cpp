#include <expected>

#include <glfw3webgpu.h>
#include <webgpu/webgpu.hpp>

#include "../glfw/global.h"
#include "global.h"
#include "util.h"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::Surface, Error> CreateSurface(::wgpu::Instance& instance,
                                                    GLFWwindow* window) {
  auto surface = glfwCreateWindowWGPUSurface(instance, window);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  if (auto e = glfw::global::error_stack.Pop()) return std::unexpected(*e);
  return surface;
}

std::expected<::wgpu::SurfaceConfiguration, Error> ConfigSurface(
    ::wgpu::Surface& surface,
    ::wgpu::Adapter& adapter,
    ::wgpu::Device& device) {
  ::wgpu::SurfaceCapabilities surface_capabilities{};
  if (surface.getCapabilities(adapter, &surface_capabilities)
      != ::wgpu::Status::Success)
    return std::unexpected(Error{ "Failed to get Webgpu capabilities." });
  if (surface_capabilities.formatCount == 0)
    return std::unexpected(Error{ "No available surface format." });
  ::wgpu::SurfaceConfiguration surface_config =
      [&] -> ::wgpu::SurfaceConfiguration {
    ::wgpu::SurfaceConfiguration config;
    config.nextInChain = nullptr;
    config.width = 640;
    config.height = 480;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.format = surface_capabilities.formats[0];
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;
    config.device = device;
    // config.presentMode = WGPUPresentMode_Fifo; //< with VSync
    config.presentMode = WGPUPresentMode_Immediate; //< no vsync
    config.alphaMode = ::wgpu::CompositeAlphaMode::Auto;
    return config;
  }();
  surface.configure(surface_config);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return surface_config;
}


std::expected<::wgpu::TextureView, Error> NextTargetView(
    ::wgpu::Surface& surface) {
  ::wgpu::SurfaceTexture surface_texture{ ::wgpu::Default };
  surface.getCurrentTexture(&surface_texture);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  if (surface_texture.status
      != ::wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal)
    return std::unexpected(
        Error(std::format("Failed to get next Webgpu surface. Status:",
                          static_cast<int>(surface_texture.status))));
  ::wgpu::TextureViewDescriptor view_desc{ [] -> ::wgpu::TextureViewDescriptor {
    ::wgpu::TextureViewDescriptor desc{ ::wgpu::Default };
    desc.label = ::wgpu::StringView{ "Surface Texture View" };
    desc.dimension = ::wgpu::TextureViewDimension::_2D;
    return desc;
  }() };
  /* Get the view. */
  ::wgpu::TextureView target_view =
      wgpuTextureCreateView(surface_texture.texture, &view_desc);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  wgpuTextureRelease(surface_texture.texture);
  return target_view;
}

}  // namespace crystal::graphics::wgpu