#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#include <expected>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"
#include "util.h"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::Adapter, Error> CreateAdapter(::wgpu::Instance& instance,
                                                    ::wgpu::Surface& surface) {
  ::wgpu::Adapter adapter =
      instance.requestAdapter([&] -> ::wgpu::RequestAdapterOptions {
        ::wgpu::RequestAdapterOptions opts;
        opts.compatibleSurface = surface;
        return opts;
      }());
  return adapter;
}

}  // namespace crystal::graphics::wgpu