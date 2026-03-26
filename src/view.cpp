#include <webgpu/webgpu.h>

#include <expected>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/api.h"
#include "global.h"

namespace crystal::graphics {

std::expected<void, Error> View(const Scene& scene, const Camera& camera) {
  if (!global::env)
    return std::unexpected(Error{ "Environment not initialized." });
  return global::env->View(scene, camera);
}

} // namespace crystal::graphics
