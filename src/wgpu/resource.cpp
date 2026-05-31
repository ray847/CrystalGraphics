#include "resource.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/camera.h"
#include "global.h"
#include "src/image_decode.h"
#include "src/material.h"
#include "src/pathtracing/blas.h"
#include "src/pathtracing/bvh.h"
#include "src/vertex.h"
#include "webgpu/webgpu-raii.hpp"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::Buffer, Error> CreateBVHStorage(std::size_t size,
                                                      ::wgpu::Device& device);
std::expected<::wgpu::Texture, Error> CreateMaterialTextureArray(
    ::wgpu::Device& device, uint32_t width, uint32_t height, uint32_t layers);
std::expected<::wgpu::Sampler, Error> CreateMaterialTextureSampler(
    ::wgpu::Device& device);
std::expected<::wgpu::Texture, Error> CreateEnvironmentTexture(
    ::wgpu::Device& device, uint32_t width, uint32_t height);
std::expected<::wgpu::Sampler, Error> CreateEnvironmentTextureSampler(
    ::wgpu::Device& device);
std::expected<bool, Error> WriteMaterialTextures(const SceneData& scene_data,
                                                 Resources& resources,
                                                 ::wgpu::Queue& queue,
                                                 ::wgpu::Device& device);
std::expected<bool, Error> WriteEnvironmentTexture(const SceneData& scene_data,
                                                   Resources& resources,
                                                   ::wgpu::Queue& queue,
                                                   ::wgpu::Device& device);

namespace {

constexpr uint32_t kDefaultMaterialTextureSize = 1;
constexpr uint32_t kDefaultMaterialTextureLayers = 1;
constexpr uint32_t kDefaultEnvironmentTextureSize = 1;

struct UploadTexture {
  uint32_t width = 1;
  uint32_t height = 1;
  std::vector<std::byte> rgba8;
};

std::vector<std::byte> ResizeRgba8Nearest(const DecodedImage& image,
                                          uint32_t width,
                                          uint32_t height) {
  if (image.width == width && image.height == height) return image.rgba8;

  std::vector<std::byte> res(static_cast<std::size_t>(width) * height * 4);
  for (uint32_t y = 0; y < height; ++y) {
    uint32_t src_y =
        std::min(image.height - 1,
                 static_cast<uint32_t>(
                     (static_cast<std::uint64_t>(y) * image.height) / height));
    for (uint32_t x = 0; x < width; ++x) {
      uint32_t src_x =
          std::min(image.width - 1,
                   static_cast<uint32_t>(
                       (static_cast<std::uint64_t>(x) * image.width) / width));
      std::size_t src =
          (static_cast<std::size_t>(src_y) * image.width + src_x) * 4;
      std::size_t dst = (static_cast<std::size_t>(y) * width + x) * 4;
      std::copy_n(image.rgba8.data() + src, 4, res.data() + dst);
    }
  }
  return res;
}

std::vector<std::byte> WhiteTexture() {
  return {
    std::byte{ 255 },
    std::byte{ 255 },
    std::byte{ 255 },
    std::byte{ 255 },
  };
}

}  // namespace

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
  auto blas_storage = CreateBVHStorage(min_offset_alignment * 3 + 4, device);
  if (!blas_storage) return std::unexpected(blas_storage.error());
  /* Material Textures */
  auto material_texture_array =
      CreateMaterialTextureArray(device,
                                 kDefaultMaterialTextureSize,
                                 kDefaultMaterialTextureSize,
                                 kDefaultMaterialTextureLayers);
  if (!material_texture_array)
    return std::unexpected(material_texture_array.error());
  auto material_texture_sampler = CreateMaterialTextureSampler(device);
  if (!material_texture_sampler)
    return std::unexpected(material_texture_sampler.error());
  /* Environment Texture */
  auto environment_texture = CreateEnvironmentTexture(
      device, kDefaultEnvironmentTextureSize, kDefaultEnvironmentTextureSize);
  if (!environment_texture) return std::unexpected(environment_texture.error());
  auto environment_texture_sampler = CreateEnvironmentTextureSampler(device);
  if (!environment_texture_sampler)
    return std::unexpected(environment_texture_sampler.error());
  return Resources{
    .surface_texture = std::move(surface_texture),
    .surface_sampler = std::move(surface_sampler),
    .camera_uniform = std::move(camera_uniform),
    .tlas_storage = std::move(*tlas_storage),
    .inst_offset = min_offset_alignment,
    .scene_storage = std::move(*blas_storage),
    .idx_offset = min_offset_alignment,
    .vert_offset = min_offset_alignment * 2,
    .mat_offset = min_offset_alignment * 3,
    .material_texture_array = std::move(*material_texture_array),
    .material_texture_sampler = std::move(*material_texture_sampler),
    .environment_texture = std::move(*environment_texture),
    .environment_texture_sampler = std::move(*environment_texture_sampler),
    .material_texture_width = kDefaultMaterialTextureSize,
    .material_texture_height = kDefaultMaterialTextureSize,
    .material_texture_layers = kDefaultMaterialTextureLayers,
    .environment_texture_width = kDefaultEnvironmentTextureSize,
    .environment_texture_height = kDefaultEnvironmentTextureSize,
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

std::expected<bool, Error> AssertStorageSize(const SceneData& scene_data,
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

  const auto& bvh = scene_data.bvh_;
  const auto& vertices = scene_data.vertices_;
  const auto& materials = scene_data.materials_;

  /* TLAS Nodes & Instances */
  std::size_t tlas_nodes_size = bvh.TLAS().Nodes().size() * sizeof(TLASNode);
  std::size_t tlas_inst_size = bvh.TLAS().Instances().size() * sizeof(Instance);
  std::size_t tlas_inst_offset =
      std::max((tlas_nodes_size + min_offset_alignment - 1)
                   & ~(min_offset_alignment - 1),
               resources.inst_offset);
  std::size_t buffer_size = tlas_inst_offset + tlas_inst_size;
  auto assert_tlas_res = assert_storage(resources.tlas_storage, buffer_size);
  if (!assert_tlas_res) return std::unexpected(assert_tlas_res.error());
  bool inst_offset_change = tlas_inst_offset != resources.inst_offset;
  resources.inst_offset = tlas_inst_offset;

  /* BLAS Nodes & Indices & Vertices & Materials */
  std::size_t blas_nodes_size = bvh.BLAS().Nodes().size() * sizeof(BLASNode);
  std::size_t indices_size = bvh.BLAS().Indices().size() * sizeof(Index);
  std::size_t vertices_size = vertices.size() * sizeof(Vertex);
  std::size_t materials_size = materials.size() * sizeof(Material);
  std::size_t idx_offset = std::max((blas_nodes_size + min_offset_alignment - 1)
                                        & ~(min_offset_alignment - 1),
                                    resources.idx_offset);
  std::size_t vert_offset =
      std::max((idx_offset + indices_size + min_offset_alignment - 1)
                   & ~(min_offset_alignment - 1),
               resources.vert_offset);
  std::size_t mat_offset =
      std::max((vert_offset + vertices_size + min_offset_alignment - 1)
                   & ~(min_offset_alignment - 1),
               resources.mat_offset);
  auto assert_blas_res =
      assert_storage(resources.scene_storage, mat_offset + materials_size);
  bool idx_offset_change = idx_offset != resources.idx_offset;
  resources.idx_offset = idx_offset;
  bool vert_offset_change = vert_offset != resources.vert_offset;
  resources.vert_offset = vert_offset;
  bool mat_offset_change = mat_offset != resources.mat_offset;
  resources.mat_offset = mat_offset;
  if (!assert_blas_res) return std::unexpected(assert_blas_res.error());
  return *assert_blas_res || *assert_tlas_res || inst_offset_change
      || idx_offset_change || vert_offset_change || mat_offset_change;
}

std::expected<SceneWriteResult, Error> WriteScene(
    const SceneData& scene_data,
    Resources& resources,
    std::size_t min_offset_alignment,
    ::wgpu::Queue& queue,
    ::wgpu::Device& device) {
  auto assert_size_res =
      AssertStorageSize(scene_data, resources, min_offset_alignment, device);
  if (!assert_size_res) return std::unexpected(assert_size_res.error());
  auto write_textures_res =
      WriteMaterialTextures(scene_data, resources, queue, device);
  if (!write_textures_res) return std::unexpected(write_textures_res.error());
  auto write_environment_res =
      WriteEnvironmentTexture(scene_data, resources, queue, device);
  if (!write_environment_res)
    return std::unexpected(write_environment_res.error());

  const auto& vertices = scene_data.vertices_;
  const auto& materials = scene_data.materials_;
  const auto& bvh = scene_data.bvh_;
  /* Write buffer. */
  /* TLAS Nodes */
  std::size_t tlas_nodes_size = bvh.TLAS().Nodes().size() * sizeof(TLASNode);
  std::size_t tlas_instances_size =
      bvh.TLAS().Instances().size() * sizeof(Instance);
  queue.writeBuffer(*resources.tlas_storage,
                    0,
                    static_cast<const void*>(bvh.TLAS().Nodes().data()),
                    tlas_nodes_size);
  /* Instances */
  queue.writeBuffer(*resources.tlas_storage,
                    resources.inst_offset,
                    static_cast<const void*>(bvh.TLAS().Instances().data()),
                    tlas_instances_size);
  /* BLAS Nodes */
  queue.writeBuffer(*resources.scene_storage,
                    0,
                    static_cast<const void*>(bvh.BLAS().Nodes().data()),
                    bvh.BLAS().Nodes().size() * sizeof(BLASNode));
  /* Indices */
  queue.writeBuffer(*resources.scene_storage,
                    resources.idx_offset,
                    static_cast<const void*>(bvh.BLAS().Indices().data()),
                    bvh.BLAS().Indices().size() * sizeof(Index));
  /* Vertices */
  queue.writeBuffer(*resources.scene_storage,
                    resources.vert_offset,
                    static_cast<const void*>(vertices.data()),
                    vertices.size() * sizeof(Vertex));
  /* Materials */
  queue.writeBuffer(*resources.scene_storage,
                    resources.mat_offset,
                    static_cast<const void*>(materials.data()),
                    materials.size() * sizeof(Material));
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return SceneWriteResult{
    .storage_changed = *assert_size_res,
    .material_textures_changed = *write_textures_res,
    .environment_texture_changed = *write_environment_res,
  };
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

std::expected<::wgpu::Texture, Error> CreateMaterialTextureArray(
    ::wgpu::Device& device, uint32_t width, uint32_t height, uint32_t layers) {
  ::wgpu::Texture texture =
      device.createTexture([&] -> ::wgpu::TextureDescriptor {
        ::wgpu::TextureDescriptor desc{ ::wgpu::Default };
        desc.size = { width, height, layers };
        desc.label =
            ::wgpu::StringView{ "Crystal Graphics Material Texture Array" };
        desc.mipLevelCount = 1;
        desc.sampleCount = 1;
        desc.dimension = ::wgpu::TextureDimension::_2D;
        desc.format = ::wgpu::TextureFormat::RGBA8Unorm;
        desc.usage = ::wgpu::TextureUsage::CopyDst
                   | ::wgpu::TextureUsage::TextureBinding;
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return texture;
}

std::expected<::wgpu::Texture, Error> CreateEnvironmentTexture(
    ::wgpu::Device& device, uint32_t width, uint32_t height) {
  ::wgpu::Texture texture =
      device.createTexture([&] -> ::wgpu::TextureDescriptor {
        ::wgpu::TextureDescriptor desc{ ::wgpu::Default };
        desc.size = { width, height, 1 };
        desc.label =
            ::wgpu::StringView{ "Crystal Graphics Environment Texture" };
        desc.mipLevelCount = 1;
        desc.sampleCount = 1;
        desc.dimension = ::wgpu::TextureDimension::_2D;
        desc.format = ::wgpu::TextureFormat::RGBA32Float;
        desc.usage = ::wgpu::TextureUsage::CopyDst
                   | ::wgpu::TextureUsage::TextureBinding;
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return texture;
}

std::expected<bool, Error> WriteMaterialTextures(const SceneData& scene_data,
                                                 Resources& resources,
                                                 ::wgpu::Queue& queue,
                                                 ::wgpu::Device& device) {
  const TextureContainer& textures = scene_data.textures_;

  std::vector<UploadTexture> uploads;
  uploads.reserve(std::max<std::size_t>(textures.size(), 1));

  uint32_t width = 1;
  uint32_t height = 1;
  uint32_t layers =
      static_cast<uint32_t>(std::max<std::size_t>(textures.size(), 1));

  if (textures.empty()) {
    uploads.push_back(UploadTexture{
        .width = 1,
        .height = 1,
        .rgba8 = WhiteTexture(),
    });
  } else {
    std::vector<DecodedImage> decoded;
    decoded.reserve(textures.size());
    for (const TextureData& texture : textures) {
      auto image = DecodeImageRgba8(texture);
      if (!image) return std::unexpected(image.error());
      width = std::max(width, image->width);
      height = std::max(height, image->height);
      decoded.push_back(std::move(*image));
    }

    for (const DecodedImage& image : decoded) {
      uploads.push_back(UploadTexture{
          .width = width,
          .height = height,
          .rgba8 = ResizeRgba8Nearest(image, width, height),
      });
    }
  }

  bool texture_changed = width != resources.material_texture_width
                      || height != resources.material_texture_height
                      || layers != resources.material_texture_layers;
  if (texture_changed) {
    auto texture = CreateMaterialTextureArray(device, width, height, layers);
    if (!texture) return std::unexpected(texture.error());
    resources.material_texture_array = ::wgpu::raii::Texture{ nullptr };
    resources.material_texture_array = std::move(*texture);
    resources.material_texture_width = width;
    resources.material_texture_height = height;
    resources.material_texture_layers = layers;
  }

  for (uint32_t layer = 0; layer < layers; ++layer) {
    const UploadTexture& upload = uploads[layer];
    ::wgpu::TexelCopyTextureInfo dst{ ::wgpu::Default };
    dst.texture = *resources.material_texture_array;
    dst.mipLevel = 0;
    dst.origin = ::wgpu::Origin3D(0, 0, layer);
    dst.aspect = ::wgpu::TextureAspect::All;

    ::wgpu::TexelCopyBufferLayout layout{ ::wgpu::Default };
    layout.offset = 0;
    layout.bytesPerRow = upload.width * 4;
    layout.rowsPerImage = upload.height;

    queue.writeTexture(dst,
                       upload.rgba8.data(),
                       upload.rgba8.size(),
                       layout,
                       ::wgpu::Extent3D(upload.width, upload.height, 1));
  }

  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return texture_changed;
}

std::expected<bool, Error> WriteEnvironmentTexture(const SceneData& scene_data,
                                                   Resources& resources,
                                                   ::wgpu::Queue& queue,
                                                   ::wgpu::Device& device) {
  const EnvironmentTextureData& texture = scene_data.environment_texture_;
  if (texture.width == 0 || texture.height == 0 || texture.rgba32f.empty())
    return std::unexpected("Environment texture has no image data.");

  bool texture_changed = texture.width != resources.environment_texture_width
                      || texture.height != resources.environment_texture_height;
  if (texture_changed) {
    auto environment_texture =
        CreateEnvironmentTexture(device, texture.width, texture.height);
    if (!environment_texture)
      return std::unexpected(environment_texture.error());
    resources.environment_texture = ::wgpu::raii::Texture{ nullptr };
    resources.environment_texture = std::move(*environment_texture);
    resources.environment_texture_width = texture.width;
    resources.environment_texture_height = texture.height;
  }

  ::wgpu::TexelCopyTextureInfo dst{ ::wgpu::Default };
  dst.texture = *resources.environment_texture;
  dst.mipLevel = 0;
  dst.origin = ::wgpu::Origin3D(0, 0, 0);
  dst.aspect = ::wgpu::TextureAspect::All;

  ::wgpu::TexelCopyBufferLayout layout{ ::wgpu::Default };
  layout.offset = 0;
  layout.bytesPerRow = texture.width * 4 * sizeof(float);
  layout.rowsPerImage = texture.height;

  queue.writeTexture(dst,
                     texture.rgba32f.data(),
                     texture.rgba32f.size() * sizeof(float),
                     layout,
                     ::wgpu::Extent3D(texture.width, texture.height, 1));
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return texture_changed;
}

std::expected<::wgpu::Sampler, Error> CreateMaterialTextureSampler(
    ::wgpu::Device& device) {
  ::wgpu::Sampler sampler =
      device.createSampler([] -> ::wgpu::SamplerDescriptor {
        ::wgpu::SamplerDescriptor desc{ ::wgpu::Default };
        desc.label =
            ::wgpu::StringView{ "Crystal Graphics Material Texture Sampler" };
        desc.magFilter = ::wgpu::FilterMode::Linear;
        desc.minFilter = ::wgpu::FilterMode::Linear;
        desc.mipmapFilter = ::wgpu::MipmapFilterMode::Nearest;
        desc.addressModeU = ::wgpu::AddressMode::Repeat;
        desc.addressModeV = ::wgpu::AddressMode::Repeat;
        desc.addressModeW = ::wgpu::AddressMode::Repeat;
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return sampler;
}

std::expected<::wgpu::Sampler, Error> CreateEnvironmentTextureSampler(
    ::wgpu::Device& device) {
  ::wgpu::Sampler sampler =
      device.createSampler([] -> ::wgpu::SamplerDescriptor {
        ::wgpu::SamplerDescriptor desc{ ::wgpu::Default };
        desc.label = ::wgpu::StringView{
          "Crystal Graphics Environment Texture Sampler"
        };
        desc.magFilter = ::wgpu::FilterMode::Nearest;
        desc.minFilter = ::wgpu::FilterMode::Nearest;
        desc.mipmapFilter = ::wgpu::MipmapFilterMode::Nearest;
        desc.addressModeU = ::wgpu::AddressMode::Repeat;
        desc.addressModeV = ::wgpu::AddressMode::ClampToEdge;
        desc.addressModeW = ::wgpu::AddressMode::ClampToEdge;
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return sampler;
}

} // namespace crystal::graphics::wgpu
