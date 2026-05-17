#include "environment.h"

#include <cstddef>
#include <expected>

#include "GLFW/glfw3.h"
#include "global.h"

namespace crystal::graphics::glfw {

std::expected<Env, Error> CreateEnv() {
  glfwSetErrorCallback([](int error_code, const char* msg) {
    global::error_stack.Push(
        Error{ std::format("GLFW error {}: {}", error_code, msg) });
  });
  if (glfwInit() == GLFW_FALSE) [[unlikely]] {
    if (auto error = global::error_stack.Pop()) return std::unexpected(*error);
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  auto window =
      glfwCreateWindow(1920, 1080, "Crystal Graphics Window", nullptr, nullptr);
  if (auto error = global::error_stack.Pop()) return std::unexpected(*error);
  return Env{ window };
}

std::expected<void, Error> Env::Sync() {
  if (!window_)
    return std::unexpected(Error{ "GLFW environment already terminated." });
  glfwPollEvents();
  if (auto error = global::error_stack.Pop()) return std::unexpected(*error);
  return {};
}

std::expected<void, Error> Env::Term() {
  if (!window_)
    return std::unexpected(Error{ "GLFW environment already terminated." });
  glfwDestroyWindow(window_);
  if (auto error = global::error_stack.Pop()) return std::unexpected(*error);
  glfwTerminate();
  if (auto error = global::error_stack.Pop()) return std::unexpected(*error);
  return {};
}

Env::operator bool() const {
  return window_ != nullptr;
}

}  // namespace crystal::graphics::glfw