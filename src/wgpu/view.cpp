#include <expected>

#include <webgpu/webgpu-raii.hpp>

#include "util.h"
#include "environment.h"

namespace crystal::graphics::wgpu {

std::expected<void, Error> Env::View() {
  auto target_view_res{ NextTargetView(*surface_) };
  if (!target_view_res) return std::unexpected(target_view_res.error());
  ::wgpu::raii::TextureView target_view{ std::move(*target_view_res) };
  /* Command Encoder */
  auto create_cmd_encoder_res{ CreateCommandEncoder(*device_) };
  if (!create_cmd_encoder_res)
    return std::unexpected(create_cmd_encoder_res.error());
  ::wgpu::raii::CommandEncoder encoder {std::move(*create_cmd_encoder_res)};
  /* Compute Pass */
  auto encode_compute_pass_res{ EncodeComputePass(
      *encoder, *compute_pipeline_, *compute_bindgroup_) };
  if (!encode_compute_pass_res)
    return std::unexpected(encode_compute_pass_res.error());
  /* Render Pass */
  auto encode_render_pass_res{ EncodeRenderPass(
      *encoder, *target_view, *render_pipeline_, *render_bindgroup_) };
  if (!encode_render_pass_res)
    return std::unexpected(encode_render_pass_res.error());
  /* Command Buffer*/
  auto create_cmd_buffer_res{ CreateCommandBuffer(*encoder) };
  if (!create_cmd_buffer_res)
    return std::unexpected(create_cmd_buffer_res.error());
  ::wgpu::raii::CommandBuffer cmd_buffer{std::move(*create_cmd_buffer_res)};
  /* Submit */
  auto submit_res = Sumbit(*queue_, *cmd_buffer);
  if (!submit_res) return std::unexpected(submit_res.error());
  return {};
}

} // namespace crystal::graphics::wgpu