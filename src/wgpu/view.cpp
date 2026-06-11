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
  /* Uniform */
  const std::uint32_t iter_count =
      frame_history_.NextIteration(scene_data.scene_tag_, camera);
  auto write_uniform_res = WriteUniform(camera, iter_count, resources_, *queue_);
  if (!write_uniform_res) return std::unexpected(write_uniform_res.error());
  /* Scene */
  if (!scene_buffer_cache_tag_.Match(scene_data)) {
    auto write_scene_res = WriteScene(scene_data,
                                      resources_,
                                      limits_.minStorageBufferOffsetAlignment,
                                      *queue_,
                                      *device_);
    if (!write_scene_res) return std::unexpected(write_scene_res.error());
    if (write_scene_res->storage_changed
        || write_scene_res->material_textures_changed
        || write_scene_res->environment_texture_changed)
      if (auto update_res =
              UpdateComputeBindGroup2(compute_bindgroups_,
                                      *resources_.tlas_storage,
                                      resources_.inst_offset,
                                      *resources_.scene_storage,
                                      resources_.idx_offset,
                                      resources_.vert_offset,
                                      resources_.mat_offset,
                                      resources_.emissive_offset,
                                      resources_.alias_offset,
                                      *resources_.material_texture_array,
                                      *resources_.material_texture_sampler,
                                      *resources_.environment_texture,
                                      *resources_.environment_texture_sampler,
                                      compute_bindgroup_layouts_,
                                      *device_);
          !update_res)
        return std::unexpected(update_res.error());
    /* Update cache. */
    scene_buffer_cache_tag_.Write(scene_data);
  }
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
      *encoder,
      *compute_pipeline_,
      compute_bindgroups_,
      conf_.render_width,
      conf_.render_height) };
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
