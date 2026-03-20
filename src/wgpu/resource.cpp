#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/camera.h"
#include "global.h"
#include "resource.h"

namespace crystal::graphics::wgpu {

std::expected<Resources, Error> CreateResources(
    const ::wgpu::SurfaceConfiguration& surface_config,
    ::wgpu::Device& device) {
  /* Surface Texture */
  ::wgpu::Texture surface_texture =
      device.createTexture([&] -> ::wgpu::TextureDescriptor {
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
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  /* Surface Sampler */
  ::wgpu::Sampler surface_sampler =
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
  /* Camera Buffer */
  ::wgpu::Buffer camera_uniform =
      device.createBuffer([] -> ::wgpu::BufferDescriptor {
        ::wgpu::BufferDescriptor desc{ ::wgpu::Default };
        desc.size = sizeof(Camera);
        desc.usage =
            ::wgpu::BufferUsage::CopyDst | ::wgpu::BufferUsage::Uniform;
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return Resources{
    .surface_texture = std::move(surface_texture),
    .surface_sampler = std::move(surface_sampler),
    .camera_uniform = std::move(camera_uniform)
  };
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

std::expected<void, Error> WriteCameraUniform(const Camera& camera,
                                              Resources& resources,
                                              ::wgpu::Queue& queue) {
  queue.writeBuffer(*resources.camera_uniform,
                    0,
                    static_cast<const void*>(&camera),
                    sizeof(Camera));
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return {};
}

} // namespace crystal::graphics::wgpu