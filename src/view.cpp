#include <webgpu/webgpu.h>

#include <expected>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/api.h"
#include "pathtracing/scene_data.h"
#include "global.h"

namespace crystal::graphics {

std::expected<void, Error> View(const Scene& scene, const Camera& camera) {
  if (!global::env)
    return std::unexpected(Error{ "Environment not initialized." });
  /* Process the scene data. */
  SceneData scene_data{scene};
  return global::env->View(camera);
}

} // namespace crystal::graphics
