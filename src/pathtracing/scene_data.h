#ifndef CRYSTALGRAPHICS_PATHTRACING_SCENE_DATA_H_
#define CRYSTALGRAPHICS_PATHTRACING_SCENE_DATA_H_

#include <vector>

#include "CrystalGraphics/scene.h"
#include "bvh.h"
#include "src/scene_impl.h"

namespace crystal::graphics {

class SceneData {
 public:
  /* Variables */
  const std::vector<Vertex>& vertices_;
  BVH bvh_;

  /* Constructor */
  SceneData(const Scene& scene) : vertices_(scene.impl_->vertices_), bvh_(scene) {
  }
};

} // namespace crystal::graphics

#endif
