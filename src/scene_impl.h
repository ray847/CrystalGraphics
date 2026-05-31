#ifndef CRYSTALGRAPHICS_SRC_SCENE_IMPL_H_
#define CRYSTALGRAPHICS_SRC_SCENE_IMPL_H_

#include <CrystalSpatial/spatial.h>

#include <cassert>
#include <cstdint>
#include <ranges>
#include <vector>

#include "CrystalGraphics/public.h"
#include "CrystalGraphics/scene.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float4.hpp"
#include "material.h"
#include "texture.h"
#include "vertex.h"

namespace crystal::graphics {

struct Primitive {
  size32_t vertex_offset;
  size32_t vertex_count;
  size32_t index_offset;
  size32_t index_count;
  size32_t material_idx;
};

struct Scene::Impl {
  struct Trans : public crystal::spatial::TRSQuatTrans {
    using crystal::spatial::TRSQuatTrans::operator();
    using CompleteTrans = crystal::spatial::AffineTrans<3, float>;
    using Mat = glm::mat4;

    bool use_matrix = false;
    Mat matrix = Mat(1.0f);

    void SetMatrix(const Mat& mat) {
      matrix = mat;
      use_matrix = true;
    }

    Mat Matrix() const {
      if (use_matrix) return matrix;
      return crystal::spatial::TRSQuatTrans::Matrix();
    }

    Vec operator()(const Vec& v) const {
      return Matrix() * glm::vec4{ v, 1.0f };
    }

    template <std::ranges::range VecView>
    auto operator()(const VecView& vv) const {
      auto mat = Matrix();
      return vv | std::views::transform([mat](const Vec& v) -> Vec {
               return mat * glm::vec4{ v, 1.0f };
             });
    }

    CompleteTrans operator()(const Trans& t) const {
      return { Matrix() * t.Matrix() };
    }

    operator CompleteTrans() const {
      return { Matrix() };
    }

    Primitive operator()(const Primitive& prim) const {
      assert(false && "No transforming meshes for now.");
      return prim;
    }
  };

  using SpaceDef = spatial::SpaceDef<Trans, Primitive>;

  VertexContainer vertices_;
  std::vector<size32_t> indices_;
  std::vector<Material> materials_;
  TextureContainer textures_;
  spatial::Space<SpaceDef> space_;
};

}  // namespace crystal::graphics

#endif
