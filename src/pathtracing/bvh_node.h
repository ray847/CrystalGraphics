#ifndef CRYSTALGRAPHICS_PATHTRACING_BVH_NODE_H_
#define CRYSTALGRAPHICS_PATHTRACING_BVH_NODE_H_

#include <cstdint>
#include <limits>

#include "glm/ext/vector_float3.hpp"

namespace crystal::graphics {

struct alignas(32) TLASNode {
  glm::vec3 lb = {
    std::numeric_limits<float>::max(),
    std::numeric_limits<float>::max(),
    std::numeric_limits<float>::max(),
  };
  union {
    uint32_t child = 0;
    uint32_t primitive_offset;
  };
  glm::vec3 ub = {
    std::numeric_limits<float>::min(),
    std::numeric_limits<float>::min(),
    std::numeric_limits<float>::min(),
  };
  uint32_t primitive_count = 0;
};

} // namespace crystal::graphics

#endif