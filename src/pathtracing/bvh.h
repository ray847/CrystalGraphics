#ifndef CRYSTALGRAPHICS_SRC_PATHTRACING_BVH_H_
#define CRYSTALGRAPHICS_SRC_PATHTRACING_BVH_H_

#include <CrystalSpatial/spatial.h>

#include <filesystem>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <unordered_map>

#include "CrystalGraphics/scene.h"
#include "blas.h"
#include "tlas.h"

namespace crystal::graphics {

class BVH {
 public:
  /* Constructor */
  BVH(const Scene& scene) :
      blas_([&] -> class BLAS& {
        if (!cached_blas_.contains(scene.FilePath())) {
          cached_blas_.insert(
              { scene.FilePath(), std::make_unique<class BLAS>(scene) });
        }
        return *cached_blas_.at(scene.FilePath());
      }()),
      tlas_(scene, blas_.Roots()) {
  }

  /* Accessor */
  const auto& TLAS() const {
    return tlas_;
  }
  const auto& BLAS() const {
    return blas_;
  }

 private:
  inline static std::unordered_map<std::filesystem::path,
                                   std::unique_ptr<class BLAS>>
      cached_blas_{};
  class BLAS& blas_;
  class TLAS tlas_;
};

}  // namespace crystal::graphics

#endif
