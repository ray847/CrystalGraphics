#ifndef CRYSTALGRAPHICS_SRC_PATHTRACING_BVH_H_
#define CRYSTALGRAPHICS_SRC_PATHTRACING_BVH_H_

#include <CrystalSpatial/spatial.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/vec3.hpp>

#include "CrystalGraphics/scene.h"
#include "blas.h"
#include "tlas.h"


namespace crystal::graphics {

using Primitive = Scene::Primitive;

class BVH {
 public:
  /* Constructor */
  BVH(const Scene& scene) : tlas_(scene), blas_(scene) {
  }

  /* Accessor */
  const auto& TLAS() const {
    return tlas_;
  }
  const auto& BLAS() const {
    return blas_;
  }

 private:
  class TLAS tlas_;
  class BLAS blas_;
};

}  // namespace crystal::graphics

#endif