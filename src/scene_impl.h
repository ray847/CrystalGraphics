#ifndef CRYSTALGRAPHICS_SRC_SCENE_IMPL_H_
#define CRYSTALGRAPHICS_SRC_SCENE_IMPL_H_

#include <CrystalSpatial/spatial.h>

#include <cassert>
#include <cstdint>
#include <vector>

#include "CrystalGraphics/public.h"
#include "CrystalGraphics/scene.h"
#include "vertex.h"

namespace crystal::graphics {

struct Primitive {
  size32_t vertex_offset;
  size32_t vertex_count;
  size32_t index_offset;
  size32_t index_count;
};

struct Scene::Impl {
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

  VertexContainer vertices_;
  std::vector<uint32_t> indices_;
  spatial::Space<SpaceDef> space_;
};

}  // namespace crystal::graphics

#endif
