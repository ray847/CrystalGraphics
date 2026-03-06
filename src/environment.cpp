#include <expected>
#include <sstream>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu.hpp>
#include <webgpu/webgpu-raii.hpp>
#include <glfw3webgpu.h>

#include "CrystalGraphics/environment.h"
#include "CrystalGraphics/error.h"
#include "error_stack.h"

namespace crystal::graphics {

/* Implementation Functions */
std::expected<void, Error> InitGLFW();
std::expected<void, Error> TermGLFW();
std::expected<void, Error> SyncGLFW();
std::expected<void, Error> InitWGPU();
std::expected<void, Error> TermWGPU();
std::expected<void, Error> SyncWGPU();

namespace global {
bool env_status = false;
} // namespace global
/**
 * Initialize glfw & webgpu.
 */
std::expected<void, Error> EnvInit() {
  auto init_glfw_res = InitGLFW();
  if (!init_glfw_res) return std::unexpected(init_glfw_res.error());
  auto init_wgpu_res = InitWGPU();
  if (!init_wgpu_res) return std::unexpected(init_wgpu_res.error());
  global::env_status = true;
  return {};
}
std::expected<void, Error> EnvTerm() {
  auto term_wgpu_res = TermWGPU();
  if (!term_wgpu_res) return std::unexpected(term_wgpu_res.error());
  auto term_glfw_res = TermGLFW();
  if (!term_glfw_res) return std::unexpected(term_glfw_res.error());
  global::env_status = false;
  return {};
}
std::expected<void, Error> EnvSync() {
  auto sync_wgpu_res = SyncWGPU();
  if (!sync_wgpu_res) return std::unexpected(sync_wgpu_res.error());
  auto sync_glfw_res = SyncGLFW();
  if (!sync_glfw_res) return std::unexpected(sync_glfw_res.error());
  return {};
}
bool EnvStatus() {
  return global::env_status;
}

namespace global {
GLFWwindow* glfw_window;
}
std::expected<void, Error> InitGLFW() {
  glfwSetErrorCallback([](int error_code, const char* msg) {
    global::error_stack.Push(
        Error{std::format("GLFW error {}: {}", error_code, msg)});
  });
  if (glfwInit() == GLFW_FALSE) [[unlikely]] {
    if (auto error = global::error_stack.Pop())
      return std::unexpected(*error);
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  global::glfw_window =
      glfwCreateWindow(640, 480, "Crystal Graphics Window", nullptr, nullptr);
  return {};
}
std::expected<void, Error> TermGLFW() {
  glfwDestroyWindow(global::glfw_window);
  if (auto error = global::error_stack.Pop())
    return std::unexpected(*error);
  glfwTerminate();
  if (auto error = global::error_stack.Pop())
    return std::unexpected(*error);
  return {};
}
std::expected<void, Error> SyncGLFW() {
  glfwPollEvents();
  if (auto error = global::error_stack.Pop())
    return std::unexpected(*error);
  return {};
}

namespace global {
wgpu::raii::Instance wgpu_instance;
wgpu::raii::Device wgpu_device;
wgpu::raii::Surface wgpu_surface;
} // namespace global
std::expected<void, Error> InitWGPU() {
  global::wgpu_instance = wgpu::createInstance();
  global::wgpu_surface = wgpu::Surface{ glfwCreateWindowWGPUSurface(
      *global::wgpu_instance, global::glfw_window) };
  wgpu::RequestAdapterOptions adapter_opts;
  adapter_opts.compatibleSurface = *global::wgpu_surface;
  wgpu::raii::Adapter adapter =
      global::wgpu_instance->requestAdapter(adapter_opts);
  wgpu::Limits limits{};
  adapter->getLimits(&limits);
  wgpu::DeviceDescriptor device_desc{};
  device_desc.requiredLimits = &limits;
  wgpu::DeviceLostCallbackInfo device_lost_callback{};
  device_lost_callback.mode = wgpu::CallbackMode::AllowSpontaneous;
  device_lost_callback.callback = [](const WGPUDevice* device,
                                     WGPUDeviceLostReason reason,
                                     WGPUStringView msg,
                                     void*,
                                     void*) {
    if (reason == wgpu::DeviceLostReason::Destroyed) return;
    std::stringstream ss;
    ss << "WebGPU device lost:\n"
       << "Reason: " << reason << '\n'
       << "Message" << wgpu::StringView{ msg } << '\n';
    global::error_stack.Push(Error{ss.str()});
  };
  wgpu::UncapturedErrorCallbackInfo uncaptured_error_callback{};
  uncaptured_error_callback.callback = [](const WGPUDevice* device,
                                          WGPUErrorType type,
                                          WGPUStringView msg,
                                          void*,
                                          void*) {
    std::stringstream ss;
    ss << "Uncaptured error:\n"
       << "Error Type: " << type << '\n'
       << "Message" << wgpu::StringView{ msg } << '\n';
    global::error_stack.Push(Error{ss.str()});
  };
  device_desc.deviceLostCallbackInfo = device_lost_callback;
  device_desc.uncapturedErrorCallbackInfo = uncaptured_error_callback;
  global::wgpu_device = adapter->createDevice(device_desc);
  global::wgpu_instance->processEvents();
  /* Configure surface. */
  wgpu::SurfaceCapabilities surface_capabilities{};
  if (global::wgpu_surface->getCapabilities(*adapter, &surface_capabilities)
      != wgpu::Status::Success)
    return std::unexpected(Error{ "Failed to get Webgpu capabilities." });
  wgpu::SurfaceConfiguration surface_config{};
  surface_config.nextInChain = nullptr;
  glfwGetWindowSize(global::glfw_window,
                    reinterpret_cast<int*>(&surface_config.width),
                    reinterpret_cast<int*>(&surface_config.height));
  surface_config.usage = WGPUTextureUsage_RenderAttachment;
  if (surface_capabilities.formatCount == 0)
    return std::unexpected(Error{ "No available surface format." });
  surface_config.format = surface_capabilities.formats[0];
  surface_config.viewFormatCount = 0;
  surface_config.viewFormats = nullptr;
  surface_config.device = *global::wgpu_device;
  surface_config.presentMode = WGPUPresentMode_Fifo;
  surface_config.alphaMode = WGPUCompositeAlphaMode_Auto;

  wgpuSurfaceConfigure(*global::wgpu_surface, &surface_config);
  return {};
}
std::expected<void, Error> TermWGPU() {
  global::wgpu_surface->unconfigure();
  global::wgpu_surface = wgpu::raii::Surface{ nullptr };
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  global::wgpu_device = wgpu::raii::Device{ nullptr };
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  global::wgpu_instance = wgpu::raii::Instance{ nullptr };
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return {};
}
std::expected<void, Error> SyncWGPU() {
  global::wgpu_instance->processEvents();
  if (auto error = global::error_stack.Pop()) return std::unexpected(*error);
  return {};
}

} // namespace crystal::graphics
