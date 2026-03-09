#include <expected>
#include <sstream>
#include <array>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu.hpp>
#include <webgpu/webgpu-raii.hpp>
#include <glfw3webgpu.h>

#include "CrystalGraphics/environment.h"
#include "error_stack.h"
#include "global.h"

namespace crystal::graphics {

/* Implementation Functions */
std::expected<void, Error> InitGLFW();
std::expected<void, Error> TermGLFW();
std::expected<void, Error> SyncGLFW();
std::expected<void, Error> InitWGPU();
std::expected<void, Error> TermWGPU();
std::expected<void, Error> SyncWGPU();

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
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  global::glfw_window =
      glfwCreateWindow(1440, 1080, "Crystal Graphics Window", nullptr, nullptr);
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

std::expected<wgpu::Adapter, Error> InitWGPUAdapter() {
  wgpu::Adapter adapter =
      global::wgpu_instance->requestAdapter([] -> wgpu::RequestAdapterOptions {
        wgpu::RequestAdapterOptions opts;
        opts.compatibleSurface = *global::wgpu_surface;
        return opts;
      }());
  return adapter;
}
std::expected<wgpu::Device, Error> InitWGPUDevice(wgpu::Adapter& adapter) {
  auto limits = [&] -> wgpu::Limits {
    wgpu::Limits limits = wgpu::Default;
    adapter.getLimits(&limits);
    return limits;
  }();
  wgpu::Device device = adapter.createDevice([&] -> wgpu::DeviceDescriptor {
    wgpu::DeviceDescriptor desc = wgpu::Default;
    desc.requiredLimits = &limits;
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
      global::error_stack.Push(Error{ ss.str() });
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
      global::error_stack.Push(Error{ ss.str() });
    };
    desc.deviceLostCallbackInfo = device_lost_callback;
    desc.uncapturedErrorCallbackInfo = uncaptured_error_callback;
    return desc;
  }());
  return device;
}
std::expected<wgpu::SurfaceConfiguration, Error> GetWGPUSurfaceConfig(
    wgpu::Adapter& adapter) {
  wgpu::SurfaceCapabilities surface_capabilities{};
  if (global::wgpu_surface->getCapabilities(adapter, &surface_capabilities)
      != wgpu::Status::Success)
    return std::unexpected(Error{ "Failed to get Webgpu capabilities." });
  if (surface_capabilities.formatCount == 0)
    return std::unexpected(Error{ "No available surface format." });
  wgpu::SurfaceConfiguration surface_config =
      [&] -> wgpu::SurfaceConfiguration {
    wgpu::SurfaceConfiguration config;
    config.nextInChain = nullptr;
    glfwGetWindowSize(global::glfw_window,
                      reinterpret_cast<int*>(&config.width),
                      reinterpret_cast<int*>(&config.height));
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.format = surface_capabilities.formats[0];
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;
    config.device = *global::wgpu_device;
    //config.presentMode = WGPUPresentMode_Fifo; //< with VSync
    config.presentMode = WGPUPresentMode_Immediate; //< no vsync
    config.alphaMode = wgpu::CompositeAlphaMode::Auto;
    return config;
  }();
  return surface_config;
}
std::expected<wgpu::Texture, Error> InitWGPUSurfaceTextureBuffer() {
  int width, height;
  glfwGetWindowSize(global::glfw_window, &width, &height);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return global::wgpu_device->createTexture([&] -> wgpu::TextureDescriptor {
    wgpu::TextureDescriptor desc = wgpu::Default;
    desc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    desc.mipLevelCount = 1;
    desc.sampleCount = 1;
    desc.dimension = wgpu::TextureDimension::_2D;
    desc.format = global::wgpu_surface_format;
    desc.usage = wgpu::TextureUsage::TextureBinding;
    return desc;
  }());
}
std::expected<wgpu::BindGroupLayout, Error> InitWGPUBindGroupLayout() {
  std::array<wgpu::BindGroupLayoutEntry, 2> entries{
    /* View Port Texture */
    [] -> wgpu::BindGroupLayoutEntry {
      wgpu::BindGroupLayoutEntry entry = wgpu::Default;
      entry.binding = 0;
      entry.visibility =
          wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment;
      entry.texture.sampleType = wgpu::TextureSampleType::Float;
      entry.texture.viewDimension = wgpu::TextureViewDimension::_2D;
      return entry;
    }(),
    /* Sampler for View Port Texture */
    [] -> wgpu::BindGroupLayoutEntry {
      wgpu::BindGroupLayoutEntry entry = wgpu::Default;
      entry.binding = 1;
      entry.visibility =
          wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment;
      entry.sampler.type = wgpu::SamplerBindingType::Filtering;
      return entry;
    }(),
  };
  return global::wgpu_device->createBindGroupLayout(
      [&] -> wgpu::BindGroupLayoutDescriptor {
        wgpu::BindGroupLayoutDescriptor desc = wgpu::Default;
        desc.entryCount = entries.size();
        desc.entries = entries.data();
        return desc;
      }());
}
std::expected<wgpu::RenderPipeline, Error> InitWGPURenderPipeline() {
  /* Vertex Shader */
  wgpu::ShaderSourceWGSL vert_src_desc = wgpu::Default;
  constexpr char vert_src[] = {
#embed "shader/vertex.wgsl"
    , '\0'
  };
  vert_src_desc.code = wgpu::StringView(vert_src);
  wgpu::ShaderModuleDescriptor vert_shader_desc = wgpu::Default;
  vert_shader_desc.nextInChain = &vert_src_desc.chain;
  vert_shader_desc.label =
      wgpu::StringView{ "Crystal Graphics Vertex Shader" };
  wgpu::raii::ShaderModule vert_shader_module =
      global::wgpu_device->createShaderModule(vert_shader_desc);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  /* Fragment Shader */
  wgpu::ShaderSourceWGSL frag_src_desc = wgpu::Default;
  constexpr char frag_src[] = {
#embed "shader/fragment.wgsl"
    , '\0'
  };
  frag_src_desc.code = wgpu::StringView(frag_src);
  wgpu::ShaderModuleDescriptor frag_shader_desc = wgpu::Default;
  frag_shader_desc.nextInChain = &frag_src_desc.chain;
  frag_shader_desc.label =
      wgpu::StringView{ "Crystal Graphics Fragment Shader" };
  wgpu::raii::ShaderModule frag_shader_module =
      global::wgpu_device->createShaderModule(frag_shader_desc);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  /* Render Pipeline */
  wgpu::RenderPipelineDescriptor render_pipeline_desc = wgpu::Default;
  render_pipeline_desc.vertex.module = *vert_shader_module;
  render_pipeline_desc.vertex.entryPoint = wgpu::StringView{ "vert_main" };
  wgpu::FragmentState fragment_state = wgpu::Default;
  fragment_state.module = *frag_shader_module;
  fragment_state.entryPoint = wgpu::StringView{ "frag_main" };
  wgpu::ColorTargetState color_target_state = wgpu::Default;
  color_target_state.format = global::wgpu_surface_format;
  wgpu::BlendState blend_state = wgpu::Default;
  color_target_state.blend = &blend_state;
  fragment_state.targetCount = 1;
  fragment_state.targets = &color_target_state;
  render_pipeline_desc.fragment = &fragment_state;
  wgpu::RenderPipeline pipeline =
      global::wgpu_device->createRenderPipeline(render_pipeline_desc);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return pipeline;
}
std::expected<wgpu::ComputePipeline, Error> InitWGPUComputePipeline() {
}
std::expected<void, Error> InitWGPU() {
  /* Instance */
  global::wgpu_instance = wgpu::createInstance();
  /* Surface */
  global::wgpu_surface = wgpu::Surface{ glfwCreateWindowWGPUSurface(
      *global::wgpu_instance, global::glfw_window) };
  /* Adapter (temporary) */
  wgpu::raii::Adapter adapter;
  auto init_adapter_res = InitWGPUAdapter();
  if (init_adapter_res) adapter = std::move(*init_adapter_res);
  else return std::unexpected(init_adapter_res.error());
  /* Device */
  if (auto init_device_res = InitWGPUDevice(*adapter))
    global::wgpu_device = std::move(*init_device_res);
  else return std::unexpected(init_device_res.error());
  /* Surface Config */
  wgpu::SurfaceConfiguration surface_config;
  auto get_surface_config_res = GetWGPUSurfaceConfig(*adapter);
  if (get_surface_config_res) surface_config = *get_surface_config_res;
  else return std::unexpected(get_surface_config_res.error());
  global::wgpu_surface->configure(surface_config);
  global::wgpu_surface_format = surface_config.format;
  /* Queue */
  global::wgpu_queue = global::wgpu_device->getQueue();
  /* Surface Texture */
  //auto init_surface_texture_res = InitWGPUSurfaceTextureBuffer();
  //if (init_surface_texture_res)
  //  global::wgpu_surface_texture = std::move(*init_surface_texture_res);
  //else return std::unexpected(init_adapter_res.error());
  /* Render Pipeline */
  auto init_pipeline_res = InitWGPURenderPipeline();
  if (init_pipeline_res)
    global::wgpu_render_pipeline = std::move(*init_pipeline_res);
  else return std::unexpected(init_pipeline_res.error());
  return {};
}
std::expected<void, Error> TermWGPU() {
  global::wgpu_queue = wgpu::raii::Queue{nullptr};
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
