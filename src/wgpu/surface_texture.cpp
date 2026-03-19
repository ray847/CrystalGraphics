#include <glfw3webgpu.h>

#include <expected>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"
#include "global.h"
#include "util.h"

namespace crystal::graphics::wgpu {
std::expected<::wgpu::Texture, Error> CreateSurfaceTexture(
    ::wgpu::Device& device,
    const ::wgpu::SurfaceConfiguration& surface_config) {
  return device.createTexture([&] -> ::wgpu::TextureDescriptor {
    ::wgpu::TextureDescriptor desc{ ::wgpu::Default };
    desc.size = { surface_config.width, surface_config.height, 1 };
    desc.label = ::wgpu::StringView{ "Crystal Graphics Surface Texture" };
    desc.mipLevelCount = 1;
    desc.sampleCount = 1;
    desc.dimension = ::wgpu::TextureDimension::_2D;
    desc.format = ::wgpu::TextureFormat::RGBA8Unorm;
    desc.usage = ::wgpu::TextureUsage::TextureBinding
               | ::wgpu::TextureUsage::StorageBinding;
    return desc;
  }());
}

std::expected<::wgpu::Sampler, Error> CreateSurfaceTextureSampler(
    ::wgpu::Device& device) {
  ::wgpu::Sampler sampler =
      device.createSampler([] -> ::wgpu::SamplerDescriptor {
        ::wgpu::SamplerDescriptor desc{ ::wgpu::Default };
        desc.label =
            ::wgpu::StringView{ "Crystal Graphics Surface Texture Sampler" };
        desc.magFilter = ::wgpu::FilterMode::Linear;
        desc.minFilter = ::wgpu::FilterMode::Linear;
        desc.mipmapFilter = ::wgpu::MipmapFilterMode::Nearest;
        desc.addressModeU = ::wgpu::AddressMode::ClampToEdge;
        desc.addressModeV = ::wgpu::AddressMode::ClampToEdge;
        desc.addressModeW = ::wgpu::AddressMode::ClampToEdge;
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return sampler;
}

std::expected<::wgpu::TextureView, Error> CreateSurfaceTextureView(
    ::wgpu::Texture& surface_texture) {
  ::wgpu::TextureViewDescriptor surface_texture_view_desc =
      [&] -> ::wgpu::TextureViewDescriptor {
    ::wgpu::TextureViewDescriptor desc{ ::wgpu::Default };
    desc.nextInChain = nullptr;
    desc.label =
        ::wgpu::StringView{ "Crystal Graphics Surface Texture View (Compute)" };
    desc.format = surface_texture.getFormat();
    desc.dimension = ::wgpu::TextureViewDimension::_2D;
    desc.baseMipLevel = 0;
    desc.mipLevelCount = 1;
    desc.baseArrayLayer = 0;
    desc.arrayLayerCount = 1;
    desc.aspect = ::wgpu::TextureAspect::All;
    return desc;
  }();
  ::wgpu::TextureView surface_texture_view{ surface_texture.createView(
      surface_texture_view_desc) };
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return surface_texture_view;
}
}  // namespace crystal::graphics::wgpu