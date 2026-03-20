#include <webgpu/webgpu.hpp>

#include "instance.h"

namespace crystal::graphics::wgpu {
std::expected<::wgpu::Instance, Error> CreateInstance() {
  return ::wgpu::createInstance();
}
}  // namespace crystal::graphics::wgpu