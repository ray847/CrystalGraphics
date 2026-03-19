#include <webgpu/webgpu.h>

#include <expected>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "global.h"
#include "CrystalGraphics/api.h"

namespace crystal::graphics {

std::expected<void, Error> View(const Scene& scene) {
  if (!global::env)
    return std::unexpected(Error{ "Environment not initialized." });
  return global::env->View();
}

} // namespace crystal::graphics
