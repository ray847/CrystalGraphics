#ifndef CRYSTALGRAPHICS_SRC_PATHTRACING_AABB_H_
#define CRYSTALGRAPHICS_SRC_PATHTRACING_AABB_H_

#include <limits>
#include <vector>

#include <glm/common.hpp>
#include <glm/ext/vector_float3.hpp>

#include "CrystalGraphics/mesh.h"
#include "CrystalGraphics/vertex.h"

namespace crystal::graphics {

class AABB {
 public:
  /* Variables */
  glm::vec3 lb_ = { std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max() };
  glm::vec3 ub_ = { std::numeric_limits<float>::min(),
                    std::numeric_limits<float>::min(),
                    std::numeric_limits<float>::min() };

  /* Constructor */
  AABB() = default;
  AABB(glm::vec3 lb, glm::vec3 ub) : lb_(lb), ub_(ub) {
  }
  AABB(const Mesh& mesh, const VertexContainer& vertices) {
    for (uint32_t i = mesh.vertex_offset;
         i < mesh.vertex_offset + mesh.vertex_count;
         ++i) {
      Merge(vertices[i].position);
    }
  }

  /* Functions */
  void Merge(const glm::vec3& v) {
    lb_ = glm::min(lb_, v);
    ub_ = glm::max(ub_, v);
  }
  void Merge(const AABB& other) {
    lb_ = glm::min(lb_, other.lb_);
    ub_ = glm::max(ub_, other.ub_);
  }
  glm::vec3 Center() const {
    return (lb_ + ub_) * 0.5f; // assume no overflow
  }
  glm::vec3 Range() const {
    return ub_ - lb_;
  }
  float Area() const {
    auto r = Range();
    return 2.0f * (r.y * r.z + r.x * r.z + r.x * r.y);
  }
};

} // namespace crystal::graphics

#endif