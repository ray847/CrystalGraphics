#ifndef CRYSTALGRAPHICS_SCENE_H_
#define CRYSTALGRAPHICS_SCENE_H_

#include <expected>
#include <filesystem>

#include <glm/common.hpp>
#include <CrystalSpatial/spatial.h>

#include "CrystalSpatial/transformation/affine.h"
#include "vertex.h"
#include "mesh.h"
#include "error.h"

namespace crystal::graphics {

class Scene {
 public:

 private:
  /* Space Definition */
  struct Trans : public crystal::spatial::TRSQuatTrans {
    using crystal::spatial::TRSQuatTrans::operator();
    crystal::spatial::AffineTrans<3, float> operator()(const Trans& t) const {
      return { Matrix() * t.Matrix() };
    }
    Mesh operator()(const Mesh& mesh) const {
      assert(false && "No transforming meshes for now.");
      return mesh;
    }
  };
  using SpaceDef = spatial::SpaceDef<Trans, Mesh>;

  /* Variables */
  VertexContainer vertices_;
  std::vector<uint32_t> indicies_;
  spatial::Space<SpaceDef> space_;

  /* Constructor */
  Scene() = default;
  friend std::expected<Scene, Error> LoadScene(std::filesystem::path file);
  friend class BVH;
};

std::expected<Scene, Error> LoadScene(std::filesystem::path file);

} // namespace crystal::graphics

#endif
