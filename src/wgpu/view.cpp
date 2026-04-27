#include <expected>
#include <webgpu/webgpu-raii.hpp>

#include "CrystalGraphics/camera.h"
#include "command.h"
#include "compute.h"
#include "environment.h"
#include "queue.h"
#include "render.h"
#include "resource.h"
#include "src/pathtracing/scene_data.h"
#include "surface.h"

namespace crystal::graphics::wgpu {

std::expected<void, Error> Env::View(const SceneData& scene_data,
                                     const Camera& camera) {
  /* Write Inputs */
  /* Camera */
  auto write_camera_res = WriteCameraUniform(camera, resources_, *queue_);
  if (!write_camera_res) return std::unexpected(write_camera_res.error());
  /* BVH */
  auto write_bvh_res = WriteScene(scene_data,
                                  resources_,
                                  limits_.minStorageBufferOffsetAlignment,
                                  *queue_,
                                  *device_);
  if (!write_bvh_res) return std::unexpected(write_bvh_res.error());
  if (*write_bvh_res)
    if (auto update_res = UpdateComputeBindGroup2(compute_bindgroups_,
                                                  *resources_.tlas_storage,
                                                  resources_.inst_offset,
                                                  *resources_.blas_storage,
                                                  resources_.idx_offset,
                                                  resources_.vert_offset,
                                                  resources_.mat_offset,
                                                  compute_bindgroup_layouts_,
                                                  *device_);
        !update_res)
      return std::unexpected(update_res.error());
  /* Target View */
  auto target_view_res{ NextTargetView(*surface_) };
  if (!target_view_res) return std::unexpected(target_view_res.error());
  ::wgpu::raii::TextureView target_view{ std::move(*target_view_res) };
  /* Command Encoder */
  auto create_cmd_encoder_res{ CreateCommandEncoder(*device_) };
  if (!create_cmd_encoder_res)
    return std::unexpected(create_cmd_encoder_res.error());
  ::wgpu::raii::CommandEncoder encoder{ std::move(*create_cmd_encoder_res) };
  /* Compute Pass */
  auto encode_compute_pass_res{ EncodeComputePass(
      *encoder, *compute_pipeline_, compute_bindgroups_) };
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
  ::wgpu::raii::CommandBuffer cmd_buffer{ std::move(*create_cmd_buffer_res) };
  /* Submit */
  auto submit_res = Sumbit(*queue_, *cmd_buffer);
  if (!submit_res) return std::unexpected(submit_res.error());
  return {};
}

} // namespace crystal::graphics::wgpu
