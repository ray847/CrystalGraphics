#ifndef CRYSTALGRAPHICS_PATHTRACING_SCENE_DATA_H_
#define CRYSTALGRAPHICS_PATHTRACING_SCENE_DATA_H_

#include "CrystalGraphics/scene.h"
#include "bvh.h"

namespace crystal::graphics {

class SceneData {
 public:
  /* Variables */
  BVH bvh_;

  /* Constructor */
  SceneData(const Scene& scene) : bvh_(scene) {
  }
};

} // namespace crystal::graphics

#endif