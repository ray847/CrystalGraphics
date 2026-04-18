#include "adapter.h"

#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#include <expected>
#include <iostream>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::Adapter, Error> CreateAdapter(::wgpu::Instance& instance,
                                                    ::wgpu::Surface& surface) {
  ::wgpu::Adapter adapter =
      instance.requestAdapter([&] -> ::wgpu::RequestAdapterOptions {
        ::wgpu::RequestAdapterOptions opts;
        opts.compatibleSurface = surface;
        opts.powerPreference = ::wgpu::PowerPreference::HighPerformance;
        opts.backendType = ::wgpu::BackendType::Vulkan;
        return opts;
      }());
  ::wgpu::AdapterInfo info{};
  adapter.getInfo(&info);
  std::clog << ::wgpu::StringView{ info.description } << std::endl;
  std::clog << ::wgpu::StringView{ info.vendor } << std::endl;
  std::clog << ::wgpu::StringView{ info.architecture } << std::endl;
  std::clog << ::wgpu::StringView{ info.device } << std::endl;
  return adapter;
}

}  // namespace crystal::graphics::wgpu