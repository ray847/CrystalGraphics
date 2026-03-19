#include <array>

#include <expected>
#include <webgpu/webgpu.hpp>

#include "global.h"
#include "util.h"
#include "webgpu/webgpu-raii.hpp"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::BindGroupLayout, Error> CreateRenderBindGroupLayout(
    ::wgpu::Device& device) {
  std::array<::wgpu::BindGroupLayoutEntry, 2> entries {
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry entry{::wgpu::Default};
      entry.binding = 0;
      entry.visibility = ::wgpu::ShaderStage::Fragment;
      entry.texture.sampleType = ::wgpu::TextureSampleType::Float;
      entry.texture.viewDimension = ::wgpu::TextureViewDimension::_2D;
      entry.texture.multisampled = false;
      return entry;
    }(),
    [&] -> ::wgpu::BindGroupLayoutEntry {
      ::wgpu::BindGroupLayoutEntry entry{::wgpu::Default};
      entry.binding = 1;
      entry.visibility = ::wgpu::ShaderStage::Fragment;
      entry.sampler.type = ::wgpu::SamplerBindingType::Filtering;
      return entry;
    }(),
 };
 auto bindgroup_layout = device.createBindGroupLayout(
  [&] -> ::wgpu::BindGroupLayoutDescriptor {
    ::wgpu::BindGroupLayoutDescriptor desc{::wgpu::Default};
    desc.entries = entries.data();
    desc.entryCount = entries.size();
    desc.label = ::wgpu::StringView{"Crystal Graphics Render BindGroup Layout"};
    return desc;
  }()
 );
 if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
 return bindgroup_layout;
}

std::expected<::wgpu::BindGroup, Error> CreateRenderBindGroup(
  ::wgpu::TextureView& surface_texture_view,
  ::wgpu::Sampler& surface_texture_sampler,
  ::wgpu::BindGroupLayout& layout,
  ::wgpu::Device& device
) {
  std::array<::wgpu::BindGroupEntry, 2> entries {
    [&] -> ::wgpu::BindGroupEntry {
      ::wgpu::BindGroupEntry entry{::wgpu::Default};
      entry.binding = 0,
      entry.textureView = surface_texture_view;
      return entry;
    }(),
    [&]-> ::wgpu::BindGroupEntry{
      ::wgpu::BindGroupEntry entry{::wgpu::Default};
      entry.binding = 1;
      entry.sampler = surface_texture_sampler;
      return entry;
    }(),
  };
  ::wgpu::BindGroup bindgroup {
    device.createBindGroup([&] -> ::wgpu::BindGroupDescriptor {
      ::wgpu::BindGroupDescriptor desc{ ::wgpu::Default };
      desc.entries = entries.data();
      desc.entryCount = entries.size();
      desc.label = ::wgpu::StringView{ "Crystal Graphics Render BindGroup" };
      desc.layout = layout;
      desc.nextInChain = nullptr;
      return desc;
    }())
  };
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return bindgroup;
}

std::expected<::wgpu::RenderPipeline, Error> CreateRenderPipeline(
  ::wgpu::BindGroupLayout& bindgroup_layout,
  ::wgpu::SurfaceConfiguration& surface_config,
  ::wgpu::Device& device
) {
  /* Vertex Shader */
  ::wgpu::ShaderSourceWGSL vert_src_desc{ ::wgpu::Default };
  constexpr char vert_src[] = {
#embed "shader/vertex.wgsl"
    , '\0'
  };
  vert_src_desc.code = ::wgpu::StringView(vert_src);
  ::wgpu::ShaderModuleDescriptor vert_shader_desc{ ::wgpu::Default };
  vert_shader_desc.nextInChain = &vert_src_desc.chain;
  vert_shader_desc.label =
      ::wgpu::StringView{ "Crystal Graphics Vertex Shader" };
  ::wgpu::raii::ShaderModule vert_shader_module =
      device.createShaderModule(vert_shader_desc);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  /* Fragment Shader */
  ::wgpu::ShaderSourceWGSL frag_src_desc{::wgpu::Default};
  constexpr char frag_src[] = {
#embed "shader/fragment.wgsl"
    , '\0'
  };
  frag_src_desc.code = ::wgpu::StringView(frag_src);
  ::wgpu::ShaderModuleDescriptor frag_shader_desc{::wgpu::Default};
  frag_shader_desc.nextInChain = &frag_src_desc.chain;
  frag_shader_desc.label =
      ::wgpu::StringView{ "Crystal Graphics Fragment Shader" };
  ::wgpu::raii::ShaderModule frag_shader_module =
      device.createShaderModule(frag_shader_desc);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  /* Pipeline Layout */
  ::wgpu::raii::PipelineLayout pipeline_layout{ device.createPipelineLayout(
      [&] -> ::wgpu::PipelineLayoutDescriptor {
        ::wgpu::PipelineLayoutDescriptor desc{::wgpu::Default};
        desc.bindGroupLayoutCount = 1;
        desc.bindGroupLayouts =
            reinterpret_cast<WGPUBindGroupLayout*>(&bindgroup_layout);
        return desc;
      }()) };
  /* Pipeline */
  ::wgpu::RenderPipelineDescriptor render_pipeline_desc{::wgpu::Default};
  render_pipeline_desc.layout = *pipeline_layout;
  render_pipeline_desc.vertex.module = *vert_shader_module;
  render_pipeline_desc.vertex.entryPoint = ::wgpu::StringView{ "vert_main" };
  ::wgpu::FragmentState fragment_state{::wgpu::Default};
  fragment_state.module = *frag_shader_module;
  fragment_state.entryPoint = ::wgpu::StringView{ "frag_main" };
  ::wgpu::ColorTargetState color_target_state = ::wgpu::Default;
  color_target_state.format = surface_config.format;
  ::wgpu::BlendState blend_state{::wgpu::Default};
  color_target_state.blend = &blend_state;
  fragment_state.targetCount = 1;
  fragment_state.targets = &color_target_state;
  render_pipeline_desc.fragment = &fragment_state;
  ::wgpu::RenderPipeline pipeline =
      device.createRenderPipeline(render_pipeline_desc);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return pipeline;
}

std::expected<void, Error> EncodeRenderPass(::wgpu::CommandEncoder& encoder,
                                            ::wgpu::TextureView& target_view,
                                            ::wgpu::RenderPipeline& pipeline,
                                            ::wgpu::BindGroup& bindgroup) {
  ::wgpu::RenderPassDescriptor render_pass_desc{ ::wgpu::Default };
  auto color_attachment = [&] -> ::wgpu::RenderPassColorAttachment {
    ::wgpu::RenderPassColorAttachment color_attachment = ::wgpu::Default;
    color_attachment.view = target_view;
    color_attachment.loadOp = ::wgpu::LoadOp::Clear;
    color_attachment.storeOp = ::wgpu::StoreOp::Store;
    color_attachment.clearValue = ::wgpu::Color{ 0.0f, 0.5f, 0.8f, 1.0f };
    return color_attachment;
  }();
  render_pass_desc.colorAttachmentCount = 1;
  render_pass_desc.colorAttachments = &color_attachment;
  ::wgpu::raii::RenderPassEncoder render_pass =
      encoder.beginRenderPass(render_pass_desc);
  render_pass->setPipeline(pipeline);
  render_pass->setBindGroup(
      /* group index */ 0,
      /* bindgroup */ bindgroup,
      /* dynamic offset count */ 0,
      /* dynamic offsets */ nullptr);
  render_pass->draw(3, 1, 0, 0);
  render_pass->end();
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return {};
}
}