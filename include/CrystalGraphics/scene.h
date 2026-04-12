#ifndef CRYSTALGRAPHICS_SCENE_H_
#define CRYSTALGRAPHICS_SCENE_H_

#include <CrystalSpatial/spatial.h>

#include <expected>
#include <filesystem>
#include <glm/common.hpp>

#include "error.h"
#include "public.h"
#include "vertex.h"

namespace crystal::graphics {

class Scene {
 public:
  struct Primitive {
    size32_t vertex_offset;
    size32_t vertex_count;
    size32_t index_offset;
    size32_t index_count;
  };
  const auto& FilePath() const {
    return filepath_;
  }

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
  const std::filesystem::path filepath_;
  VertexContainer vertices_;
  std::vector<uint32_t> indicies_;
  spatial::Space<SpaceDef> space_;

  /* Constructor */
  Scene(const std::filesystem::path& filepath) : filepath_(filepath) {
  }
  friend std::expected<Scene, Error> LoadScene(std::filesystem::path file);
  friend class BVH;
  friend class TLAS;
  friend class BLAS;
};

std::expected<Scene, Error> LoadScene(std::filesystem::path file);

} // namespace crystal::graphics

#endif
