#include "limits.h"

#include <expected>

#include "global.h"
#include "webgpu/webgpu.hpp"

namespace crystal::graphics::wgpu {

/* Instance */
std::expected<::wgpu::Limits, Error> CreateLimits(::wgpu::Adapter& adapter) {
  ::wgpu::Limits limits;
  adapter.getLimits(&limits);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return limits;
}

}  // namespace crystal::graphics::wgpu