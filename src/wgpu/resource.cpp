#include <cmath>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/camera.h"
#include "global.h"
#include "src/pathtracing/bvh.h"
#include "src/pathtracing/bvh_node.h"
#include "resource.h"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::Buffer, Error> CreateBVHStorage(std::size_t size,
                                                      ::wgpu::Device& device);

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
  /* BVH Storage */
  auto bvh_storage = CreateBVHStorage(4 , device);
  if (!bvh_storage) return std::unexpected(bvh_storage.error());
  return Resources{
    .surface_texture = std::move(surface_texture),
    .surface_sampler = std::move(surface_sampler),
    .camera_uniform = std::move(camera_uniform),
    .bvh_storage = std::move(*bvh_storage)
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

std::expected<bool, Error> AssertBVHStorageSize(const BVH& bvh,
                                                Resources& resources,
                                                ::wgpu::Device& device) {
  auto& storage = resources.bvh_storage;
  std::size_t capacity = storage->getSize();
  std::size_t size = bvh.TLAS().size() * sizeof(TLASNode);
  if (capacity < size) [[unlikely]] { // perform resizing
    storage = ::wgpu::raii::Buffer{ nullptr }; // trigger buffer release
    auto create_storage_res =
        CreateBVHStorage(std::max(capacity * 2, size), device);
    if (!create_storage_res) return std::unexpected(create_storage_res.error());
    storage = std::move(*create_storage_res);
    if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
    return true;
  }
  return false;
}

std::expected<bool, Error> WriteBVHStorage(const BVH& bvh,
                                           Resources& resources,
                                           ::wgpu::Queue& queue,
                                           ::wgpu::Device& device) {
  auto assert_size_res = AssertBVHStorageSize(bvh, resources, device);
  if (!assert_size_res) return std::unexpected(assert_size_res.error());
  /* Write buffer. */
  queue.writeBuffer(*resources.bvh_storage,
                    0,
                    static_cast<const void*>(bvh.TLAS().data()),
                    bvh.TLAS().size() * sizeof(TLASNode));
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return *assert_size_res;
}

std::expected<::wgpu::Buffer, Error> CreateBVHStorage(std::size_t size,
                                                      ::wgpu::Device& device) {
  ::wgpu::Buffer bvh_storage =
      device.createBuffer([size] -> ::wgpu::BufferDescriptor {
        ::wgpu::BufferDescriptor desc{ ::wgpu::Default };
        desc.size = size;
        desc.usage =
            ::wgpu::BufferUsage::CopyDst | ::wgpu::BufferUsage::Storage;
        desc.label = ::wgpu::StringView{"BVH Storage Buffer"};
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return bvh_storage;
}

} // namespace crystal::graphics::wgpu