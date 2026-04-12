#ifndef CRYSTALGRAPHICS_SCENE_H_
#define CRYSTALGRAPHICS_SCENE_H_

#include <expected>
#include <filesystem>

#include <glm/common.hpp>
#include <CrystalSpatial/spatial.h>

#include "public.h"
#include "vertex.h"
#include "error.h"

namespace crystal::graphics {

class Scene {
 public:
  struct Primitive {
    ssize_t vertex_offset;
    ssize_t vertex_count;
    ssize_t index_offset;
    ssize_t index_count;
  };

 private:
  /* Space Definition */
  struct Trans : public crystal::spatial::TRSQuatTrans {
    using crystal::spatial::TRSQuatTrans::operator();
    crystal::spatial::AffineTrans<3, float> operator()(const Trans& t) const {
      return { Matrix() * t.Matrix() };
    }
    Primitive operator()(const Primitive& prim) const {
      assert(false && "No transforming meshes for now.");
      return prim;
    }
  };
  using SpaceDef = spatial::SpaceDef<Trans, Primitive>;

  /* Variables */
  VertexContainer vertices_;
  std::vector<uint32_t> indicies_;
  spatial::Space<SpaceDef> space_;

  /* Constructor */
  Scene() = default;
  friend std::expected<Scene, Error> LoadScene(std::filesystem::path file);
  friend class BVH;
  friend class TLAS;
  friend class BLAS;
};

std::expected<Scene, Error> LoadScene(std::filesystem::path file);

} // namespace crystal::graphics

#endif
