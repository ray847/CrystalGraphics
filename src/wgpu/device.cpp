#include <sstream>

#include "global.h"
#include "device.h"
#include "webgpu/webgpu.hpp"

namespace crystal::graphics::wgpu {
std::expected<::wgpu::Device, Error> CreateDevice(::wgpu::Adapter& adapter) {
  auto limits = [&] -> ::wgpu::Limits {
    ::wgpu::Limits limits = ::wgpu::Default;
    adapter.getLimits(&limits);
    return limits;
  }();
  ::wgpu::Device device = adapter.createDevice([&] -> ::wgpu::DeviceDescriptor {
    ::wgpu::DeviceDescriptor desc = ::wgpu::Default;
    desc.requiredLimits = &limits;
    ::wgpu::DeviceLostCallbackInfo device_lost_callback{};
    device_lost_callback.mode = ::wgpu::CallbackMode::AllowSpontaneous;
    device_lost_callback.callback = [](const WGPUDevice* device,
                                       WGPUDeviceLostReason reason,
                                       WGPUStringView msg,
                                       void*,
                                       void*) {
      if (reason == ::wgpu::DeviceLostReason::Destroyed) return;
      std::stringstream ss;
      ss << "WebGPU device lost:\n"
         << "Reason: " << reason << '\n'
         << "Message" << ::wgpu::StringView{ msg } << '\n';
      global::error_stack.Push(Error{ ss.str() });
    };
    ::wgpu::UncapturedErrorCallbackInfo uncaptured_error_callback{};
    uncaptured_error_callback.callback = [](const WGPUDevice* device,
                                            WGPUErrorType type,
                                            WGPUStringView msg,
                                            void*,
                                            void*) {
      std::stringstream ss;
      ss << "Uncaptured error:\n"
         << "Error Type: " << type << '\n'
         << "Message" << ::wgpu::StringView{ msg } << '\n';
      global::error_stack.Push(Error{ ss.str() });
    };
    desc.deviceLostCallbackInfo = device_lost_callback;
    desc.uncapturedErrorCallbackInfo = uncaptured_error_callback;
    return desc;
  }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return device;
}
}  // namespace crystal::graphics::wgpu