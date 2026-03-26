#ifndef CRYSTALGRAPHICS_SRC_PATHTRACING_BVH_H_
#define CRYSTALGRAPHICS_SRC_PATHTRACING_BVH_H_

#include <vector>

#include <glm/ext/matrix_float4x4.hpp>
#include <CrystalSpatial/spatial.h>

#include "CrystalGraphics/scene.h"
#include "bvh_node.h"
#include "aabb.h"

namespace crystal::graphics {

class BVH {
 public:
  /* Constructor */
  BVH(const Scene& scene);

  /* Accessor */
  const auto& TLAS() const {
    return tlas_nodes_;
  }

 private:
  struct BoundedMesh {
    AABB aabb;
    glm::vec3 center;
    uint32_t mesh_idx;
  };

  std::vector<TLASNode> tlas_nodes_;

  /* Functions */
  void Build(uint32_t node_idx,
             uint32_t l,
             uint32_t r,
             std::vector<BoundedMesh>& bounded_meshes);
};

}

#endif