#include "compute.h"

#include <webgpu/webgpu.h>

#include <algorithm>
#include <array>
#include <cmath>

#include "CrystalGraphics/camera.h"
#include "CrystalGraphics/error.h"
#include "global.h"
#include "resource.h"
#include "webgpu/webgpu.hpp"

namespace crystal::graphics::wgpu {

namespace {

constexpr std::size_t kMinStorageBindingSize = 8;

std::size_t StorageBindingSize(std::size_t size) {
  return std::max(size, kMinStorageBindingSize);
}

::wgpu::ConstantEntry PipelineConstant(const char* key, double value) {
  ::wgpu::ConstantEntry entry{ ::wgpu::Default };
  entry.key = ::wgpu::StringView{ key };
  entry.value = value;
  return entry;
}

}  // namespace

struct ComputeBindGroup2 {
  ::wgpu::BindGroup bindgroup;
  ::wgpu::TextureView material_texture_array_view;
  ::wgpu::TextureView environment_texture_view;
};

std::expected<ComputeBindGroup2, Error> CreateComputeBindGroup2(
    ::wgpu::Buffer& tlas_storage,
    std::size_t tlas_inst_offset,
    ::wgpu::Buffer& blas_storage,
    std::size_t idx_offset,
    std::size_t vert_offset,
    std::size_t mat_offset,
    std::size_t emissive_offset,
    std::size_t alias_offset,
    ::wgpu::Texture& material_texture_array,
    ::wgpu::Sampler& material_texture_sampler,
    ::wgpu::Texture& environment_texture,
    ::wgpu::Sampler& environment_texture_sampler,
    ComputeBindGroupLayouts& layouts,
    ::wgpu::Device& device);

std::expected<ComputeBindGroupLayouts, Error> CreateComputeBindGroupLayouts(
    ::wgpu::Device& device) {
  /* Group 0 */
  std::array<::wgpu::BindGroupLayoutEntry, 2> group0entries{
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry surface_texture_entry{ ::wgpu::Default };
      surface_texture_entry.binding = 0;
      surface_texture_entry.visibility = ::wgpu::ShaderStage::Compute;
      surface_texture_entry.storageTexture.access =
          ::wgpu::StorageTextureAccess::WriteOnly;
      surface_texture_entry.storageTexture.format =
          ::wgpu::TextureFormat::RGBA8Unorm;
      surface_texture_entry.storageTexture.viewDimension =
          ::wgpu::TextureViewDimension::_2D;
      return surface_texture_entry;
    }(),
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry history_buffer_entry{ ::wgpu::Default };
      history_buffer_entry.binding = 1;
      history_buffer_entry.visibility = ::wgpu::ShaderStage::Compute;
      history_buffer_entry.buffer.type = ::wgpu::BufferBindingType::Storage;
      return history_buffer_entry;
    }(),
  };
  ::wgpu::BindGroupLayout group0 =
      device.createBindGroupLayout([&] -> ::wgpu::BindGroupLayoutDescriptor {
        ::wgpu::BindGroupLayoutDescriptor desc{ ::wgpu::Default };
        desc.entries = group0entries.data();
        desc.entryCount = group0entries.size();
        desc.label = ::wgpu::StringView{
          "Crystal Graphics Compute BindGroup Layout Group 0"
        };
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);

  /* Group 1 */
  std::array<::wgpu::BindGroupLayoutEntry, 1> group1entries{
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry uniform_entry{ ::wgpu::Default };
      uniform_entry.binding = 0;
      uniform_entry.visibility = ::wgpu::ShaderStage::Compute;
      uniform_entry.buffer.type = ::wgpu::BufferBindingType::Uniform;
      uniform_entry.buffer.minBindingSize = sizeof(UniformData);
      return uniform_entry;
    }(),
  };
  ::wgpu::BindGroupLayout group1 =
      device.createBindGroupLayout([&] -> ::wgpu::BindGroupLayoutDescriptor {
        ::wgpu::BindGroupLayoutDescriptor desc{ ::wgpu::Default };
        desc.entries = group1entries.data();
        desc.entryCount = group1entries.size();
        desc.label = ::wgpu::StringView{
          "Crystal Graphics Compute BindGroup Layout Group 1"
        };
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);

  /* Group 2 */
  std::array<::wgpu::BindGroupLayoutEntry, 12> group2entries{
    /* TLAS Nodes */
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry tlas_entry{ ::wgpu::Default };
      tlas_entry.binding = 0;
      tlas_entry.visibility = ::wgpu::ShaderStage::Compute;
      tlas_entry.buffer.type = ::wgpu::BufferBindingType::ReadOnlyStorage;
      return tlas_entry;
    }(),
    /* TLAS Instances */
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry tlas_entry{ ::wgpu::Default };
      tlas_entry.binding = 1;
      tlas_entry.visibility = ::wgpu::ShaderStage::Compute;
      tlas_entry.buffer.type = ::wgpu::BufferBindingType::ReadOnlyStorage;
      return tlas_entry;
    }(),
    /* BLAS Nodes */
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry blas_entry{ ::wgpu::Default };
      blas_entry.binding = 2;
      blas_entry.visibility = ::wgpu::ShaderStage::Compute;
      blas_entry.buffer.type = ::wgpu::BufferBindingType::ReadOnlyStorage;
      return blas_entry;
    }(),
    /* Indices */
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry indices_entry{ ::wgpu::Default };
      indices_entry.binding = 3;
      indices_entry.visibility = ::wgpu::ShaderStage::Compute;
      indices_entry.buffer.type = ::wgpu::BufferBindingType::ReadOnlyStorage;
      return indices_entry;
    }(),
    /* Vertices */
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry vertices_entry{ ::wgpu::Default };
      vertices_entry.binding = 4;
      vertices_entry.visibility = ::wgpu::ShaderStage::Compute;
      vertices_entry.buffer.type = ::wgpu::BufferBindingType::ReadOnlyStorage;
      return vertices_entry;
    }(),
    /* Materials */
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry materials_entry{ ::wgpu::Default };
      materials_entry.binding = 5;
      materials_entry.visibility = ::wgpu::ShaderStage::Compute;
      materials_entry.buffer.type = ::wgpu::BufferBindingType::ReadOnlyStorage;
      return materials_entry;
    }(),
    /* Material Texture Array */
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry texture_entry{ ::wgpu::Default };
      texture_entry.binding = 6;
      texture_entry.visibility = ::wgpu::ShaderStage::Compute;
      texture_entry.texture.sampleType = ::wgpu::TextureSampleType::Float;
      texture_entry.texture.viewDimension =
          ::wgpu::TextureViewDimension::_2DArray;
      texture_entry.texture.multisampled = false;
      return texture_entry;
    }(),
    /* Material Texture Sampler */
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry sampler_entry{ ::wgpu::Default };
      sampler_entry.binding = 7;
      sampler_entry.visibility = ::wgpu::ShaderStage::Compute;
      sampler_entry.sampler.type = ::wgpu::SamplerBindingType::Filtering;
      return sampler_entry;
    }(),
    /* Environment Texture */
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry texture_entry{ ::wgpu::Default };
      texture_entry.binding = 8;
      texture_entry.visibility = ::wgpu::ShaderStage::Compute;
      texture_entry.texture.sampleType =
          ::wgpu::TextureSampleType::UnfilterableFloat;
      texture_entry.texture.viewDimension = ::wgpu::TextureViewDimension::_2D;
      texture_entry.texture.multisampled = false;
      return texture_entry;
    }(),
    /* Environment Texture Sampler */
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry sampler_entry{ ::wgpu::Default };
      sampler_entry.binding = 9;
      sampler_entry.visibility = ::wgpu::ShaderStage::Compute;
      sampler_entry.sampler.type = ::wgpu::SamplerBindingType::NonFiltering;
      return sampler_entry;
    }(),
    /* Emissive Primitives */
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry emissive_entry{ ::wgpu::Default };
      emissive_entry.binding = 10;
      emissive_entry.visibility = ::wgpu::ShaderStage::Compute;
      emissive_entry.buffer.type = ::wgpu::BufferBindingType::ReadOnlyStorage;
      return emissive_entry;
    }(),
    /* Emissive Alias Table */
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry alias_entry{ ::wgpu::Default };
      alias_entry.binding = 11;
      alias_entry.visibility = ::wgpu::ShaderStage::Compute;
      alias_entry.buffer.type = ::wgpu::BufferBindingType::ReadOnlyStorage;
      return alias_entry;
    }(),
  };
  ::wgpu::BindGroupLayout group2 =
      device.createBindGroupLayout([&] -> ::wgpu::BindGroupLayoutDescriptor {
        ::wgpu::BindGroupLayoutDescriptor desc{ ::wgpu::Default };
        desc.entries = group2entries.data();
        desc.entryCount = group2entries.size();
        desc.label = ::wgpu::StringView{
          "Crystal Graphics Compute BindGroup Layout Group 2"
        };
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);

  return ComputeBindGroupLayouts{ group0, group1, group2 };
}

std::expected<ComputeBindGroups, Error> CreateComputeBindGroups(
    /* Group 0 */
    ::wgpu::TextureView& surface_texture_view,
    ::wgpu::Buffer& history_buffer,
    /* Group 1 */
    ::wgpu::Buffer& uniform,
    /* Group 2 */
    ::wgpu::Buffer& tlas_storage,
    std::size_t insts_offset,
    ::wgpu::Buffer& blas_idx_vert_storage,
    std::size_t idx_offset,
    std::size_t vert_offset,
    std::size_t mat_offset,
    std::size_t emissive_offset,
    std::size_t alias_offset,
    ::wgpu::Texture& material_texture_array,
    ::wgpu::Sampler& material_texture_sampler,
    ::wgpu::Texture& environment_texture,
    ::wgpu::Sampler& environment_texture_sampler,
    ComputeBindGroupLayouts& layouts,
    ::wgpu::Device& device) {
  /* Group 0 */
  std::array<::wgpu::BindGroupEntry, 2> group0entries{
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry surface_texture{ ::wgpu::Default };
      surface_texture.binding = 0;
      surface_texture.textureView = surface_texture_view;
      return surface_texture;
    }(),
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry history{ ::wgpu::Default };
      history.binding = 1;
      history.buffer = history_buffer;
      history.size = history_buffer.getSize();
      return history;
    }(),
  };
  ::wgpu::BindGroup group0{
    device.createBindGroup([&] -> ::wgpu::BindGroupDescriptor {
      ::wgpu::BindGroupDescriptor desc{ ::wgpu::Default };
      desc.entries = group0entries.data();
      desc.entryCount = group0entries.size();
      desc.label =
          ::wgpu::StringView{ "Crystal Graphics BindGroup Group 0 (Compute)" };
      desc.layout = layouts[0];
      desc.nextInChain = nullptr;
      return desc;
    }())
  };
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);

  /* Group 1 */
  std::array<::wgpu::BindGroupEntry, 1> group1entries{
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry uniform_entry{ ::wgpu::Default };
      uniform_entry.binding = 0;
      uniform_entry.buffer = uniform;
      uniform_entry.size = uniform.getSize();
      return uniform_entry;
    }(),
  };
  ::wgpu::BindGroup group1{
    device.createBindGroup([&] -> ::wgpu::BindGroupDescriptor {
      ::wgpu::BindGroupDescriptor desc{ ::wgpu::Default };
      desc.entries = group1entries.data();
      desc.entryCount = group1entries.size();
      desc.label =
          ::wgpu::StringView{ "Crystal Graphics BindGroup Group 1 (Compute)" };
      desc.layout = layouts[1];
      desc.nextInChain = nullptr;
      return desc;
    }())
  };
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);

  auto group2 = CreateComputeBindGroup2(tlas_storage,
                                        insts_offset,
                                        blas_idx_vert_storage,
                                        idx_offset,
                                        vert_offset,
                                        mat_offset,
                                        emissive_offset,
                                        alias_offset,
                                        material_texture_array,
                                        material_texture_sampler,
                                        environment_texture,
                                        environment_texture_sampler,
                                        layouts,
                                        device);
  if (!group2) return std::unexpected(group2.error());
  return ComputeBindGroups{ group0,
                            group1,
                            group2->bindgroup,
                            group2->material_texture_array_view,
                            group2->environment_texture_view };
}

std::expected<void, Error> UpdateComputeBindGroup2(
    ComputeBindGroups& bindgroups,
    ::wgpu::Buffer& tlas_storage,
    std::size_t insts_offset,
    ::wgpu::Buffer& blas_idx_vert_storage,
    std::size_t idx_offset,
    std::size_t vert_offset,
    std::size_t mat_offset,
    std::size_t emissive_offset,
    std::size_t alias_offset,
    ::wgpu::Texture& material_texture_array,
    ::wgpu::Sampler& material_texture_sampler,
    ::wgpu::Texture& environment_texture,
    ::wgpu::Sampler& environment_texture_sampler,
    ComputeBindGroupLayouts& layouts,
    ::wgpu::Device& device) {
  auto group2 = CreateComputeBindGroup2(tlas_storage,
                                        insts_offset,
                                        blas_idx_vert_storage,
                                        idx_offset,
                                        vert_offset,
                                        mat_offset,
                                        emissive_offset,
                                        alias_offset,
                                        material_texture_array,
                                        material_texture_sampler,
                                        environment_texture,
                                        environment_texture_sampler,
                                        layouts,
                                        device);
  if (!group2) return std::unexpected(group2.error());
  if (bindgroups[2]) bindgroups[2].release();
  if (bindgroups.material_texture_array_view)
    bindgroups.material_texture_array_view.release();
  if (bindgroups.environment_texture_view)
    bindgroups.environment_texture_view.release();
  bindgroups[2] = group2->bindgroup;
  bindgroups.material_texture_array_view = group2->material_texture_array_view;
  bindgroups.environment_texture_view = group2->environment_texture_view;
  return {};
}

std::expected<::wgpu::ComputePipeline, Error> CreateComputePipeline(
    ComputeBindGroupLayouts& bindgroup_layouts, ::wgpu::Device& device) {
  /* Compute Shader */
  ::wgpu::ShaderSourceWGSL comp_src_desc{ ::wgpu::Default };
  constexpr char comp_src[] = {
#embed "shader/compute.wgsl"
    , '\0'
  };
  comp_src_desc.code = ::wgpu::StringView(comp_src);
  ::wgpu::ShaderModuleDescriptor comp_shader_desc{ ::wgpu::Default };
  comp_shader_desc.nextInChain = &comp_src_desc.chain;
  comp_shader_desc.label =
      ::wgpu::StringView{ "Crystal Graphics Compute Shader" };
  ::wgpu::raii::ShaderModule comp_shader_module =
      device.createShaderModule(comp_shader_desc);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);

  /* Pipeline Layout */
  ::wgpu::raii::PipelineLayout pipeline_layout{ device.createPipelineLayout(
      [&] -> ::wgpu::PipelineLayoutDescriptor {
        ::wgpu::PipelineLayoutDescriptor desc{ ::wgpu::Default };
        desc.bindGroupLayoutCount = bindgroup_layouts.size();
        desc.bindGroupLayouts = bindgroup_layouts.data();
        desc.label =
            ::wgpu::StringView{ "Crystal Graphics Pipeline Layout (Compute)" };
        return desc;
      }()) };

  /* Pipeline */
  std::array<::wgpu::ConstantEntry, 9> constants{
    PipelineConstant(global::kResolutionWidthOverrideId,
                     static_cast<double>(global::kResolutionWidth)),
    PipelineConstant(global::kResolutionHeightOverrideId,
                     static_cast<double>(global::kResolutionHeight)),
    PipelineConstant(global::kRenderSampleCountOverrideId,
                     static_cast<double>(global::kRenderSampleCount)),
    PipelineConstant(global::kLodMaxDepthOverrideId,
                     static_cast<double>(global::kLodMaxDepth)),
    PipelineConstant(global::kTraceMaxDepthOverrideId,
                     static_cast<double>(global::kTraceMaxDepth)),
    PipelineConstant(global::kMaxTransportSampleCountOverrideId,
                     static_cast<double>(global::kMaxTransportSampleCount)),
    PipelineConstant(global::kMaxEmissionSampleCountOverrideId,
                     static_cast<double>(global::kMaxEmissionSampleCount)),
    PipelineConstant(global::kMaxDiffuseSampleCountOverrideId,
                     static_cast<double>(global::kMaxDiffuseSampleCount)),
    PipelineConstant(global::kMaxRoughSampleCountOverrideId,
                     static_cast<double>(global::kMaxRoughSampleCount)),
  };
  ::wgpu::ComputePipeline pipeline{ device.createComputePipeline(
      [&constants,
       &comp_shader_module,
       &pipeline_layout] -> ::wgpu::ComputePipelineDescriptor {
        ::wgpu::ComputePipelineDescriptor desc{ ::wgpu::Default };
        desc.compute.constantCount = constants.size();
        desc.compute.constants = constants.data();
        desc.compute.entryPoint = ::wgpu::StringView{ "main" };
        desc.compute.module = *comp_shader_module;
        desc.layout = *pipeline_layout;
        return desc;
      }()) };
  return pipeline;
}

std::expected<void, Error> EncodeComputePass(::wgpu::CommandEncoder& encoder,
                                             ::wgpu::ComputePipeline& pipeline,
                                             ComputeBindGroups& bindgroups) {
  ::wgpu::raii::ComputePassEncoder compute_pass_encoder{
    encoder.beginComputePass([] -> ::wgpu::ComputePassDescriptor {
      ::wgpu::ComputePassDescriptor desc{ ::wgpu::Default };
      desc.timestampWrites = nullptr;
      return desc;
    }())
  };
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  compute_pass_encoder->setPipeline(pipeline);
  compute_pass_encoder->setBindGroup(
      /* group index */ 0,
      /* bindgroup */ bindgroups[0],
      /* dynamic offset count */ 0,
      /* dynamic offsets */ nullptr);
  compute_pass_encoder->setBindGroup(
      /* group index */ 1,
      /* bindgroup */ bindgroups[1],
      /* dynamic offset count */ 0,
      /* dynamic offsets */ nullptr);
  compute_pass_encoder->setBindGroup(
      /* group index */ 2,
      /* bindgroup */ bindgroups[2],
      /* dynamic offset count */ 0,
      /* dynamic offsets */ nullptr);
  uint32_t width = global::kResolutionWidth;
  uint32_t height = global::kResolutionHeight;
  uint32_t workgroup_size_x = 16;
  uint32_t workgroup_size_y = 16;
  uint32_t workgroup_count_x =
      std::ceil(static_cast<double>(width) / workgroup_size_x);
  uint32_t workgroup_count_y =
      std::ceil(static_cast<double>(height) / workgroup_size_y);
  compute_pass_encoder->dispatchWorkgroups(
      workgroup_count_x, workgroup_count_y, 1);
  compute_pass_encoder->end();
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return {};
}

std::expected<ComputeBindGroup2, Error> CreateComputeBindGroup2(
    ::wgpu::Buffer& tlas_storage,
    std::size_t tlas_inst_offset,
    ::wgpu::Buffer& blas_idx_vert_storage,
    std::size_t idx_offset,
    std::size_t vert_offset,
    std::size_t mat_offset,
    std::size_t emissive_offset,
    std::size_t alias_offset,
    ::wgpu::Texture& material_texture_array,
    ::wgpu::Sampler& material_texture_sampler,
    ::wgpu::Texture& environment_texture,
    ::wgpu::Sampler& environment_texture_sampler,
    ComputeBindGroupLayouts& layouts,
    ::wgpu::Device& device) {
  ::wgpu::TextureView material_texture_array_view =
      material_texture_array.createView([] -> ::wgpu::TextureViewDescriptor {
        ::wgpu::TextureViewDescriptor desc{ ::wgpu::Default };
        desc.label = ::wgpu::StringView{
          "Crystal Graphics Material Texture Array View"
        };
        desc.format = ::wgpu::TextureFormat::RGBA8Unorm;
        desc.dimension = ::wgpu::TextureViewDimension::_2DArray;
        desc.baseMipLevel = 0;
        desc.mipLevelCount = 1;
        desc.baseArrayLayer = 0;
        desc.arrayLayerCount = WGPU_ARRAY_LAYER_COUNT_UNDEFINED;
        desc.aspect = ::wgpu::TextureAspect::All;
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);

  ::wgpu::TextureView environment_texture_view =
      environment_texture.createView([] -> ::wgpu::TextureViewDescriptor {
        ::wgpu::TextureViewDescriptor desc{ ::wgpu::Default };
        desc.label =
            ::wgpu::StringView{ "Crystal Graphics Environment Texture View" };
        desc.format = ::wgpu::TextureFormat::RGBA32Float;
        desc.dimension = ::wgpu::TextureViewDimension::_2D;
        desc.baseMipLevel = 0;
        desc.mipLevelCount = 1;
        desc.baseArrayLayer = 0;
        desc.arrayLayerCount = 1;
        desc.aspect = ::wgpu::TextureAspect::All;
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) {
    material_texture_array_view.release();
    return std::unexpected(*e);
  }

  std::array<::wgpu::BindGroupEntry, 12> group2entries{
    /* TLAS Nodes */
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry bvh{ ::wgpu::Default };
      bvh.binding = 0;
      bvh.buffer = tlas_storage;
      bvh.size = StorageBindingSize(tlas_inst_offset);
      return bvh;
    }(),
    /* Instances */
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry bvh{ ::wgpu::Default };
      bvh.binding = 1;
      bvh.buffer = tlas_storage;
      bvh.offset = tlas_inst_offset;
      bvh.size = StorageBindingSize(tlas_storage.getSize() - tlas_inst_offset);
      return bvh;
    }(),
    /* BLAS Nodes */
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry bvh{ ::wgpu::Default };
      bvh.binding = 2;
      bvh.buffer = blas_idx_vert_storage;
      bvh.offset = 0;
      bvh.size = StorageBindingSize(idx_offset);
      return bvh;
    }(),
    /* Indices */
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry indices{ ::wgpu::Default };
      indices.binding = 3;
      indices.buffer = blas_idx_vert_storage;
      indices.offset = idx_offset;
      indices.size = StorageBindingSize(vert_offset - idx_offset);
      return indices;
    }(),
    /* Vertices */
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry vertices{ ::wgpu::Default };
      vertices.binding = 4;
      vertices.buffer = blas_idx_vert_storage;
      vertices.offset = vert_offset;
      vertices.size = StorageBindingSize(mat_offset - vert_offset);
      return vertices;
    }(),
    /* Materials */
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry materials{ ::wgpu::Default };
      materials.binding = 5;
      materials.buffer = blas_idx_vert_storage;
      materials.offset = mat_offset;
      materials.size = StorageBindingSize(emissive_offset - mat_offset);
      return materials;
    }(),
    /* Material Texture Array */
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry texture{ ::wgpu::Default };
      texture.binding = 6;
      texture.textureView = material_texture_array_view;
      return texture;
    }(),
    /* Material Texture Sampler */
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry sampler{ ::wgpu::Default };
      sampler.binding = 7;
      sampler.sampler = material_texture_sampler;
      return sampler;
    }(),
    /* Environment Texture */
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry texture{ ::wgpu::Default };
      texture.binding = 8;
      texture.textureView = environment_texture_view;
      return texture;
    }(),
    /* Environment Texture Sampler */
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry sampler{ ::wgpu::Default };
      sampler.binding = 9;
      sampler.sampler = environment_texture_sampler;
      return sampler;
    }(),
    /* Emissive Primitives */
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry emissive{ ::wgpu::Default };
      emissive.binding = 10;
      emissive.buffer = blas_idx_vert_storage;
      emissive.offset = emissive_offset;
      emissive.size = StorageBindingSize(alias_offset - emissive_offset);
      return emissive;
    }(),
    /* Emissive Alias Table */
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry alias{ ::wgpu::Default };
      alias.binding = 11;
      alias.buffer = blas_idx_vert_storage;
      alias.offset = alias_offset;
      alias.size =
          StorageBindingSize(blas_idx_vert_storage.getSize() - alias_offset);
      return alias;
    }(),
  };
  ::wgpu::BindGroup group2{
    device.createBindGroup([&] -> ::wgpu::BindGroupDescriptor {
      ::wgpu::BindGroupDescriptor desc{ ::wgpu::Default };
      desc.entries = group2entries.data();
      desc.entryCount = group2entries.size();
      desc.label =
          ::wgpu::StringView{ "Crystal Graphics BindGroup Group 2 (Compute)" };
      desc.layout = layouts[2];
      desc.nextInChain = nullptr;
      return desc;
    }())
  };
  if (auto e = global::error_stack.Pop()) {
    material_texture_array_view.release();
    environment_texture_view.release();
    return std::unexpected(*e);
  }
  return ComputeBindGroup2{ group2,
                            material_texture_array_view,
                            environment_texture_view };
}

}  // namespace crystal::graphics::wgpu
