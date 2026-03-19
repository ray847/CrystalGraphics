#ifndef CRYSTALGRAPHICS_SRC_WGPU_ENVIRONMENT_H_
#define CRYSTALGRAPHICS_SRC_WGPU_ENVIRONMENT_H_

#include <expected>
#include <tuple>

#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"

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
  std::expected<void, Error> Sync();
  std::expected<void, Error> Term();
  operator bool() const;

  /* View */
  std::expected<void, Error> View();

 private:
  ::wgpu::raii::Instance instance_;
  ::wgpu::raii::Device device_;
  ::wgpu::raii::Surface surface_;
  ::wgpu::raii::Texture surface_texture_;
  ::wgpu::raii::BindGroup compute_bindgroup_;
  ::wgpu::raii::ComputePipeline compute_pipeline_;
  ::wgpu::raii::BindGroup render_bindgroup_;
  ::wgpu::raii::RenderPipeline render_pipeline_;
  ::wgpu::raii::Queue queue_;
  /* Constructor */
  Env(::wgpu::Instance&& instance,
           ::wgpu::Device&& device,
           ::wgpu::Surface&& surface,
           ::wgpu::Texture&& surface_texture,
           ::wgpu::BindGroup&& compute_bindgroup,
           ::wgpu::ComputePipeline&& compute_pipeline,
           ::wgpu::BindGroup&& render_bindgroup,
           ::wgpu::RenderPipeline&& render_pipeline,
           ::wgpu::Queue&& queue) :
      instance_(std::move(instance)),
      device_(std::move(device)),
      surface_(std::move(surface)),
      surface_texture_(std::move(surface_texture)),
      compute_bindgroup_(std::move(compute_bindgroup)),
      compute_pipeline_(std::move(compute_pipeline)),
      render_bindgroup_(std::move(render_bindgroup)),
      render_pipeline_(std::move(render_pipeline)) {
  }
  /* Builder Function */
  friend std::expected<Env, Error> CreateEnv(GLFWwindow* window);
  friend std::expected<void, Error> View();
};

/* Builder Function */
std::expected<Env, Error> CreateEnv(GLFWwindow* window);

}  // namespace crystal::graphics::wgpu

#endif