#ifndef CRYSTALGRAPHICS_SRC_WGPU_ENVIRONMENT_H_
#define CRYSTALGRAPHICS_SRC_WGPU_ENVIRONMENT_H_

#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#include <expected>
#include <tuple>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/camera.h"
#include "CrystalGraphics/error.h"
#include "compute.h"
#include "resource.h"
#include "src/cache.h"
#include "src/pathtracing/scene_data.h"

namespace crystal::graphics::wgpu {

class Env {
 public:
  /* Rule of 5 */
  Env(const Env& other) = delete;
  Env(Env&& other) = default;
  Env& operator=(const Env& rhs) = delete;
  Env& operator=(Env&& rhs) = delete;
  ~Env() {
    std::ignore = Term();
  }
  /* Functions */
  std::expected<void, Error> Poll();
  std::expected<void, Error> Present();
  std::expected<void, Error> Term();
  operator bool() const;

  /* View */
  std::expected<void, Error> View(const SceneData& scene_data,
                                  const Camera& camera);

 private:
  ::wgpu::raii::Instance instance_;
  ::wgpu::Limits limits_;
  ::wgpu::raii::Device device_;
  ::wgpu::raii::Surface surface_;
  /* Resources */
  Resources resources_;
  struct SceneDataTagger {
    static std::filesystem::path operator()(const SceneData& scene_data) {
      return scene_data.file_path_;
    }
  };
  CacheTag<SceneData, SceneDataTagger> scene_buffer_cache_tag_{};
  /* Pipelines */
  ComputeBindGroupLayouts compute_bindgroup_layouts_;
  ComputeBindGroups compute_bindgroups_;
  ::wgpu::raii::ComputePipeline compute_pipeline_;
  ::wgpu::raii::BindGroup render_bindgroup_;
  ::wgpu::raii::RenderPipeline render_pipeline_;
  /* Queue */
  ::wgpu::raii::Queue queue_;
  /* Constructor */
  Env(::wgpu::Instance&& instance,
      ::wgpu::Limits&& limits,
      ::wgpu::Device&& device,
      ::wgpu::Surface&& surface,
      Resources&& resources,
      ComputeBindGroupLayouts&& compute_bindgroup_layouts,
      ComputeBindGroups&& compute_bindgroups,
      ::wgpu::ComputePipeline&& compute_pipeline,
      ::wgpu::BindGroup&& render_bindgroup,
      ::wgpu::RenderPipeline&& render_pipeline,
      ::wgpu::Queue&& queue) :
      instance_(std::move(instance)),
      limits_(std::move(limits)),
      device_(std::move(device)),
      surface_(std::move(surface)),
      resources_(std::move(resources)),
      compute_bindgroup_layouts_(std::move(compute_bindgroup_layouts)),
      compute_bindgroups_(std::move(compute_bindgroups)),
      compute_pipeline_(std::move(compute_pipeline)),
      render_bindgroup_(std::move(render_bindgroup)),
      render_pipeline_(std::move(render_pipeline)),
      queue_(std::move(queue)) {
  }
  /* Builder Function */
  friend std::expected<Env, Error> CreateEnv(GLFWwindow* window);
  friend std::expected<void, Error> View();
};

/* Builder Function */
std::expected<Env, Error> CreateEnv(GLFWwindow* window);

}  // namespace crystal::graphics::wgpu

#endif
