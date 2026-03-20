#include <webgpu/webgpu.h>

#include <array>
#include <ranges>

#include "CrystalGraphics/error.h"
#include "CrystalGraphics/camera.h"
#include "global.h"
#include "compute.h"
#include "webgpu/webgpu.hpp"


namespace crystal::graphics::wgpu {

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
  return ComputeBindGroupLayouts{ group0, group1 };
}

std::expected<ComputeBindGroups, Error> CreateComputeBindGroups(
    ::wgpu::TextureView& surface_texture_view,
    ::wgpu::Buffer& camera_uniform,
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
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return ComputeBindGroups{ group0, group1 };
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

}  // namespace crystal::graphics::wgpu