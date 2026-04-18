#include "resource.h"

#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/camera.h"
#include "global.h"
#include "src/pathtracing/blas.h"
#include "src/pathtracing/bvh.h"
#include "webgpu/webgpu-raii.hpp"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::Buffer, Error> CreateBVHStorage(std::size_t size,
                                                      ::wgpu::Device& device);

std::expected<Resources, Error> CreateResources(
    const ::wgpu::SurfaceConfiguration& surface_config,
    std::size_t min_offset_alignment,
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
  /* Tlas Storage */
  auto tlas_storage = CreateBVHStorage(min_offset_alignment + 4, device);
  if (!tlas_storage) return std::unexpected(tlas_storage.error());
  /* Blas Storage */
  auto blas_storage = CreateBVHStorage(4, device);
  if (!blas_storage) return std::unexpected(blas_storage.error());
  return Resources{ .surface_texture = std::move(surface_texture),
                    .surface_sampler = std::move(surface_sampler),
                    .camera_uniform = std::move(camera_uniform),
                    .tlas_storage = std::move(*tlas_storage),
                    .blas_storage = std::move(*blas_storage) };
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

std::expected<bool, Error> AssertBVHStorageSize(
    const BVH& bvh,
    Resources& resources,
    std::size_t min_offset_alignment,
    ::wgpu::Device& device) {
  auto assert_storage = [&device](
                            ::wgpu::raii::Buffer& storage,
                            std::size_t size) -> std::expected<bool, Error> {
    std::size_t capacity = storage->getSize();
    if (capacity < size) [[unlikely]] { // perform resizing
      storage = ::wgpu::raii::Buffer{ nullptr }; // trigger buffer release
      auto create_storage_res =
          CreateBVHStorage(std::max(capacity * 2, size), device);
      if (!create_storage_res)
        return std::unexpected(create_storage_res.error());
      storage = std::move(*create_storage_res);
      if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
      return true;
    }
    return false;
  };
  auto assert_blas_res = assert_storage(
      resources.blas_storage, bvh.BLAS().Nodes().size() * sizeof(BLASNode));
  if (!assert_blas_res) return std::unexpected(assert_blas_res.error());
  std::size_t tlas_nodes_size = bvh.TLAS().Nodes().size() * sizeof(TLASNode);
  std::size_t tlas_inst_size = bvh.TLAS().Instances().size() * sizeof(Instance);
  std::size_t tlas_inst_offset =
      std::max((tlas_nodes_size + min_offset_alignment - 1)
                   & ~(min_offset_alignment - 1),
               resources.tlas_inst_offset);
  std::size_t tlas_size = tlas_inst_offset + tlas_inst_size;
  auto assert_tlas_res = assert_storage(resources.tlas_storage, tlas_size);
  if (!assert_tlas_res) return std::unexpected(assert_tlas_res.error());
  bool tlas_offset_change = tlas_inst_offset != resources.tlas_inst_offset;
  resources.tlas_inst_offset = tlas_inst_offset;
  return *assert_blas_res || *assert_tlas_res || tlas_offset_change;
}

std::expected<bool, Error> WriteBVHStorage(const BVH& bvh,
                                           Resources& resources,
                                           std::size_t min_offset_alignment,
                                           ::wgpu::Queue& queue,
                                           ::wgpu::Device& device) {
  auto assert_size_res =
      AssertBVHStorageSize(bvh, resources, min_offset_alignment, device);
  if (!assert_size_res) return std::unexpected(assert_size_res.error());
  /* Write buffer. */
  /* TLAS */
  std::size_t tlas_nodes_size = bvh.TLAS().Nodes().size() * sizeof(TLASNode);
  std::size_t tlas_instances_size =
      bvh.TLAS().Instances().size() * sizeof(Instance);
  queue.writeBuffer(*resources.tlas_storage,
                    0,
                    static_cast<const void*>(bvh.TLAS().Nodes().data()),
                    tlas_nodes_size);
  /* Instances */
  queue.writeBuffer(*resources.tlas_storage,
                    resources.tlas_inst_offset,
                    static_cast<const void*>(bvh.TLAS().Instances().data()),
                    tlas_instances_size);
  /* BLAS Nodes */
  queue.writeBuffer(*resources.blas_storage,
                    0,
                    static_cast<const void*>(bvh.BLAS().Nodes().data()),
                    bvh.BLAS().Nodes().size() * sizeof(BLASNode));
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
        desc.label = ::wgpu::StringView{ "BVH Storage Buffer" };
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return bvh_storage;
}

} // namespace crystal::graphics::wgpu