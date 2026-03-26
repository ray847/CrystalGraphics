#include <webgpu/webgpu.h>

#include <array>
#include <ranges>

#include "CrystalGraphics/error.h"
#include "CrystalGraphics/camera.h"
#include "global.h"
#include "compute.h"
#include "webgpu/webgpu.hpp"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::BindGroup, Error> CreateComputeBindGroup2(
    ::wgpu::Buffer& bvh_storage,
    ComputeBindGroupLayouts& layouts,
    ::wgpu::Device& device);

std::expected<ComputeBindGroupLayouts, Error> CreateComputeBindGroupLayouts(
    ::wgpu::Device& device) {
  /* Group 0 */
  std::array<::wgpu::BindGroupLayoutEntry, 1> group0entries{
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry surface_texture_entry{::wgpu::Default};
      surface_texture_entry.binding = 0;
      surface_texture_entry.visibility = ::wgpu::ShaderStage::Compute;
      surface_texture_entry.storageTexture.access =
          ::wgpu::StorageTextureAccess::WriteOnly;
      surface_texture_entry.storageTexture.format =
          ::wgpu::TextureFormat::RGBA8Unorm;
      surface_texture_entry.storageTexture.viewDimension =
          ::wgpu::TextureViewDimension::_2D;
      return surface_texture_entry;
    }()
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
      ::wgpu::BindGroupLayoutEntry camera_entry{::wgpu::Default};
      camera_entry.binding = 0;
      camera_entry.visibility = ::wgpu::ShaderStage::Compute;
      camera_entry.buffer.type = ::wgpu::BufferBindingType::Uniform;
      camera_entry.buffer.minBindingSize = sizeof(Camera);
      return camera_entry;
    }()
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
  std::array<::wgpu::BindGroupLayoutEntry, 1> group2entries{
    /* BVH */
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry bvh_entry{::wgpu::Default};
      bvh_entry.binding = 0;
      bvh_entry.visibility = ::wgpu::ShaderStage::Compute;
      bvh_entry.buffer.type = ::wgpu::BufferBindingType::ReadOnlyStorage;
      return bvh_entry;
    }()
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
    /* Group 1 */
    ::wgpu::Buffer& camera_uniform,
    /* Group 2 */
    ::wgpu::Buffer& bvh_storage,
    ComputeBindGroupLayouts& layouts,
    ::wgpu::Device& device) {
  /* Group 0 */
  std::array<::wgpu::BindGroupEntry, 1> group0entries{
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry surface_texture{ ::wgpu::Default };
      surface_texture.binding = 0;
      surface_texture.textureView = surface_texture_view;
      return surface_texture;
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
      ::wgpu::BindGroupEntry camera{ ::wgpu::Default };
      camera.binding = 0;
      camera.buffer = camera_uniform;
      camera.size = camera_uniform.getSize();
      return camera;
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
  auto group2 = CreateComputeBindGroup2(bvh_storage, layouts, device);
  if (!group2) return std::unexpected(group2.error());
  return ComputeBindGroups{ group0, group1, *group2 };
}

std::expected<void, Error> UpdateComputeBindGroup2(
    ComputeBindGroups& bindgroups,
    ::wgpu::Buffer& bvh_storage,
    ComputeBindGroupLayouts& layouts,
    ::wgpu::Device& device) {
  auto group2 = CreateComputeBindGroup2(bvh_storage, layouts, device);
  if (!group2) return std::unexpected(group2.error());
  bindgroups[2] = std::move(*group2);
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
      ::wgpu::StringView{ "Crystal Graphics compment Shader" };
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
  ::wgpu::ComputePipeline pipeline{ device.createComputePipeline(
      [&] -> ::wgpu::ComputePipelineDescriptor {
        ::wgpu::ComputePipelineDescriptor desc{ ::wgpu::Default };
        desc.compute.constantCount = 0;
        desc.compute.constants = nullptr;
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
  for (const auto& [idx, bindgroup] : std::views::enumerate(*bindgroups)) {
    compute_pass_encoder->setBindGroup(
        /* group index */ idx,
        /* bindgroup */ bindgroup,
        /* dynamic offset count */ 0,
        /* dynamic offsets */ nullptr);
  }
  uint32_t width = global::resolution_width;
  uint32_t height = global::resolution_height;
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

std::expected<::wgpu::BindGroup, Error> CreateComputeBindGroup2(
    ::wgpu::Buffer& bvh_storage,
    ComputeBindGroupLayouts& layouts,
    ::wgpu::Device& device) {
  std::array<::wgpu::BindGroupEntry, 1> group2entries{
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry bvh{ ::wgpu::Default };
      bvh.binding = 0;
      bvh.buffer = bvh_storage;
      bvh.size = bvh_storage.getSize();
      return bvh;
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
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return group2;
}

}  // namespace crystal::graphics::wgpu