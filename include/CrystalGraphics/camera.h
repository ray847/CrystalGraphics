#ifndef CRYSTALGRAPHICS_CAMERA_H_
#define CRYSTALGRAPHICS_CAMERA_H_

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

namespace crystal::graphics {

struct Camera {
  alignas(16) glm::vec3 position{ 0, 0, 0 };
  alignas(16) glm::vec3 direction{ 1, 0, 0 };
  alignas(8) glm::vec2 viewport{ 0.5, 0.5 };
};

} // namespace crystal::graphics

#endif