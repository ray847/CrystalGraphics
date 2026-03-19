#include <webgpu/webgpu.h>

#include <array>

#include "CrystalGraphics/error.h"
#include "global.h"
#include "util.h"
#include "webgpu/webgpu.hpp"


namespace crystal::graphics::wgpu {
std::expected<::wgpu::BindGroupLayout, Error> CreateComputeBindGroupLayout(
    ::wgpu::Device& device) {
  /* BindGroup Layout */
  std::array<::wgpu::BindGroupLayoutEntry, 1> layout_entries{
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry surface_texture_entry;
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
  };
  ::wgpu::BindGroupLayout bindgroup_layout =
      device.createBindGroupLayout([&] -> ::wgpu::BindGroupLayoutDescriptor {
        ::wgpu::BindGroupLayoutDescriptor desc{ ::wgpu::Default };
        desc.entries = layout_entries.data();
        desc.entryCount = layout_entries.size();
        desc.label =
            ::wgpu::StringView{ "Crystal Graphics Compute BindGroup Layout" };
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return bindgroup_layout;
}

std::expected<::wgpu::BindGroup, Error> CreateComputeBindGroup(
    ::wgpu::TextureView& surface_texture_view,
    ::wgpu::BindGroupLayout& layout,
    ::wgpu::Device& device) {
  std::array<::wgpu::BindGroupEntry, 1> entries{ [&] -> ::wgpu::BindGroupEntry {
    ::wgpu::BindGroupEntry surface_texture{ ::wgpu::Default };
    surface_texture.binding = 0;
    surface_texture.textureView = surface_texture_view;
    return surface_texture;
  }() };
  ::wgpu::BindGroup bindgroup{
    device.createBindGroup([&] -> ::wgpu::BindGroupDescriptor {
      ::wgpu::BindGroupDescriptor desc{ ::wgpu::Default };
      desc.entries = entries.data();
      desc.entryCount = entries.size();
      desc.label = ::wgpu::StringView{ "Crystal Graphics BindGroup (Compute)" };
      desc.layout = layout;
      desc.nextInChain = nullptr;
      return desc;
    }())
  };
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return bindgroup;
}

std::expected<::wgpu::ComputePipeline, Error> CreateComputePipeline(
    ::wgpu::BindGroupLayout& bindgroup_layout, ::wgpu::Device& device) {
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
        desc.bindGroupLayoutCount = 1;
        desc.bindGroupLayouts =
            reinterpret_cast<WGPUBindGroupLayout*>(&bindgroup_layout);
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
                                             ::wgpu::BindGroup& bindgroup) {
  ::wgpu::raii::ComputePassEncoder compute_pass_encoder{ encoder.beginComputePass(
      [] -> ::wgpu::ComputePassDescriptor {
        ::wgpu::ComputePassDescriptor desc{ ::wgpu::Default };
        desc.timestampWrites = nullptr;
        return desc;
      }()) };
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  compute_pass_encoder->setPipeline(pipeline);
  compute_pass_encoder->setBindGroup(
      /* group index */ 0,
      /* bindgroup */ bindgroup,
      /* dynamic offset count */ 0,
      /* dynamic offsets */ nullptr);
  uint32_t invocation_count = 640 * 480;
  uint32_t workgroup_size = 16 * 16 * 1;
  uint32_t workgroup_count =
      std::ceil(static_cast<double>(invocation_count) / workgroup_size);
  compute_pass_encoder->dispatchWorkgroups(workgroup_count, 1, 1);
  compute_pass_encoder->end();
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return {};
}
}  // namespace crystal::graphics::wgpu