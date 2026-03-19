#ifndef CRYSTALGRAPHICS_SRC_GLFW_ENVIRONMENT_H_
#define CRYSTALGRAPHICS_SRC_GLFW_ENVIRONMENT_H_

#include <expected>
#include <tuple>

#include <GLFW/glfw3.h>

#include "CrystalGraphics/error.h"

namespace crystal::graphics::glfw {

class Env {
 public:
  /* Rule of 5 */
  Env(const Env& other) = delete;
  Env(Env&& other) : window_(other.window_) {
    other.window_ = nullptr;
  }
  Env& operator=(const Env& rhs) = delete;
  Env& operator=(Env&& rhs) = delete;
  ~Env() {
    std::ignore = Term();
  }
  /* Functions */
  std::expected<void, Error> Sync();
  std::expected<void, Error> Term();
  operator bool() const;
  /* Accessor */
  auto Window() {
    return window_;
  }

 private:
  GLFWwindow* window_;
  /* Constructor */
  Env(GLFWwindow* window) : window_(window) {
  }
  friend std::expected<Env, Error> CreateEnv();
};

std::expected<Env, Error> CreateEnv();

} // namespace crystal::graphics::glfw

#endif