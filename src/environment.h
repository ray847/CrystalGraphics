#ifndef CRYSTALGRAPHICS_SRC_ENVIRONMENT_H_
#define CRYSTALGRAPHICS_SRC_ENVIRONMENT_H_

#include <expected>
#include <tuple>

#include "glfw/environment.h"
#include "wgpu/environment.h"
#include "CrystalGraphics/error.h"
#include "CrystalGraphics/Camera.h"

namespace crystal::graphics {

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
  std::expected<void, Error> Sync();
  std::expected<void, Error> Term();

  std::expected<void, Error> View(const Camera& camera);

 private:
  glfw::Env glfw_env_;
  wgpu::Env wgpu_env_;
  Env(glfw::Env&& glfw_env, wgpu::Env&& wgpu_env) :
      glfw_env_(std::move(glfw_env)), wgpu_env_(std::move(wgpu_env)) {
  }
  friend std::expected<Env, Error> CreateEnv();
};

std::expected<Env, Error> CreateEnv();

}

#endif