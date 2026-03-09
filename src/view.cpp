#include <webgpu/webgpu.h>

#include <expected>
#include <format>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/api.h"
#include "CrystalGraphics/scene.h"
#include "error_stack.h"
#include "global.h"

namespace crystal::graphics {

std::expected<wgpu::TextureView, Error> NextSurfaceView() {
  wgpu::SurfaceTexture surface_texture = wgpu::Default;
  global::wgpu_surface->getCurrentTexture(&surface_texture);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  if (surface_texture.status
      != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal)
    return std::unexpected(
        Error(std::format("Failed to get next Webgpu surface. Status:",
                          static_cast<int>(surface_texture.status))));
  wgpu::TextureViewDescriptor view_desc = wgpu::Default;
  view_desc.label = wgpu::StringView{ "Surface Texture View" };
  view_desc.dimension = wgpu::TextureViewDimension::_2D;
  /* Get the view. */
  wgpu::TextureView target_view =
      wgpuTextureCreateView(surface_texture.texture, &view_desc);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  wgpuTextureRelease(surface_texture.texture);
  return target_view;
}

std::expected<void, Error> View(const Scene& scene) {
  auto target_view_res = NextSurfaceView();
  if (!target_view_res) return std::unexpected(target_view_res.error());
  wgpu::raii::TextureView target_view{ std::move(*target_view_res) };
  /* Command Encoder */
  wgpu::CommandEncoderDescriptor encoder_desc = wgpu::Default;
  encoder_desc.label = wgpu::StringView{ "Crystal Graphics Command Encoder" };
  wgpu::raii::CommandEncoder encoder =
      global::wgpu_device->createCommandEncoder(encoder_desc);
  /* Render Pass */
  wgpu::RenderPassDescriptor render_pass_desc = wgpu::Default;
  auto color_attachment = [&] -> wgpu::RenderPassColorAttachment {
    wgpu::RenderPassColorAttachment color_attachment = wgpu::Default;
    color_attachment.view = *target_view;
    color_attachment.loadOp = wgpu::LoadOp::Clear;
    color_attachment.storeOp = wgpu::StoreOp::Store;
    color_attachment.clearValue = wgpu::Color{ 0.0f, 0.5f, 0.8f, 1.0f };
    return color_attachment;
  }();
  render_pass_desc.colorAttachmentCount = 1;
  render_pass_desc.colorAttachments = &color_attachment;
  wgpu::raii::RenderPassEncoder render_pass =
      encoder->beginRenderPass(render_pass_desc);
  render_pass->setPipeline(*global::wgpu_render_pipeline);
  render_pass->draw(3, 1, 0, 0);
  render_pass->end();
  /* Command Buffer */
  wgpu::CommandBufferDescriptor command_buffer_desc = wgpu::Default;
  command_buffer_desc.label =
      wgpu::StringView{ "Crystal Graphics Command Buffer" };
  wgpu::raii::CommandBuffer command_buffer{ encoder->finish() };
  global::wgpu_queue->submit(*command_buffer);
  global::wgpu_surface->present();
  return {};
}

} // namespace crystal::graphics
