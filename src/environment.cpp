#include "CrystalGraphics/environment.h"

#include <memory>

#include "environment.h"
#include "glfw/environment.h"
#include "global.h"
#include "pathtracing/scene_data.h"

namespace crystal::graphics {

std::expected<void, Error> EnvInit() {
  auto env = CreateEnv();
  if (!env) return std::unexpected(env.error());
  global::env = std::make_unique<Env>(std::move(*env));
  return {};
}
std::expected<void, Error> EnvTerm() {
  auto term_res = global::env->Term();
  if (!term_res) return std::unexpected(term_res.error());
  global::env = nullptr;
  return {};
}
std::expected<void, Error> EnvSync() {
  auto sync_res = global::env->Sync();
  if (!sync_res) return std::unexpected(sync_res.error());
  return {};
}
bool EnvStatus() {
  if (!global::env) return false;
  return static_cast<bool>(global::env);
}

std::expected<Env, Error> CreateEnv() {
  auto&& glfw_env = glfw::CreateEnv();
  if (!glfw_env) return std::unexpected(glfw_env.error());
  auto&& wgpu_env = wgpu::CreateEnv(glfw_env->Window());
  if (!wgpu_env) return std::unexpected(wgpu_env.error());
  return Env{ std::move(*glfw_env), std::move(*wgpu_env) };
}

std::expected<void, Error> Env::Sync() {
  auto sync_wgpu_res = wgpu_env_.Sync();
  if (!sync_wgpu_res) return std::unexpected(sync_wgpu_res.error());
  auto sync_glfw_res = glfw_env_.Sync();
  if (!sync_glfw_res) return std::unexpected(sync_glfw_res.error());
  return {};
}

std::expected<void, Error> Env::Term() {
  auto sync_wgpu_res = wgpu_env_.Term();
  if (!sync_wgpu_res) return std::unexpected(sync_wgpu_res.error());
  auto sync_glfw_res = glfw_env_.Term();
  if (!sync_glfw_res) return std::unexpected(sync_glfw_res.error());
  return {};
}

std::expected<void, Error> Env::View(const Scene& scene, const Camera& camera) {
  SceneData scene_data{ scene };
  auto render_res = wgpu_env_.View(scene_data, camera);
  if (!render_res) return std::unexpected(render_res.error());
  return {};
}

} // namespace crystal::graphics
