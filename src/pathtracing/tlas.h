#ifndef CRYSTALGRAPHICS_SRC_PATHTRACING_TLAS_H_
#define CRYSTALGRAPHICS_SRC_PATHTRACING_TLAS_H_

#include <limits>
#include <vector>

#include <glm/ext/vector_float3.hpp>

#include "CrystalGraphics/public.h"
#include "CrystalGraphics/scene.h"
#include "aabb.h"

namespace crystal::graphics {

using Primitive = Scene::Primitive;

struct alignas(32) TLASNode {
  glm::vec3 lb = {
    std::numeric_limits<float>::max(),
    std::numeric_limits<float>::max(),
    std::numeric_limits<float>::max(),
  };
  union {
    ssize_t child = 0;
    ssize_t primitive_offset;
  };
  glm::vec3 ub = {
    std::numeric_limits<float>::min(),
    std::numeric_limits<float>::min(),
    std::numeric_limits<float>::min(),
  };
  ssize_t primitive_count = 0;
};

class TLAS {
 public:
  TLAS(const Scene& scene) {
    std::vector<BoundedPrimitive> bounded_prims;
    bounded_prims.reserve(scene.space_.ObjView<Primitive>().size());
    auto trans_aabb = [](const AABB& aabb, const glm::mat4& matrix) {
      glm::vec3 lb = glm::vec3(matrix[3]);
      glm::vec3 ub = lb;
      for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
          float a = matrix[j][i] * aabb.lb_[j];
          float b = matrix[j][i] * aabb.ub_[j];
          if (a < b) {
            lb[i] += a;
            ub[i] += b;
          } else {
            lb[i] += b;
            ub[i] += a;
          }
        }
      }
      return AABB{ lb, ub };
    };
    for (auto [i, prim] :
         std::views::enumerate(scene.space_.ObjView<Primitive>())) {
      auto world_trans = prim.SubSpaceIdx().AbsTrans();
      BoundedPrimitive res = {
        .aabb = trans_aabb(AABB{ *prim, scene.vertices_ }, world_trans.mat),
        .prim_idx = static_cast<ssize_t>(i)
      };
      res.center = res.aabb.Center();
      bounded_prims.push_back(res);
    }
    nodes_.reserve(bounded_prims.size() * 2);
    nodes_.emplace_back();
    Build(0, 0, bounded_prims.size(), bounded_prims);
  }

  const auto& Nodes() const {
    return nodes_;
  }

 private:
  struct BoundedPrimitive {
    AABB aabb;
    glm::vec3 center;
    ssize_t prim_idx;
  };

  std::vector<TLASNode> nodes_;

  void Build(ssize_t node_idx,
             ssize_t l,
             ssize_t r,
             std::vector<BoundedPrimitive>& bounded_meshes) {
    TLASNode& node = nodes_[node_idx];
    for (ssize_t i = l; i < r; ++i) {
      node.lb = glm::min(node.lb, bounded_meshes[i].aabb.lb_);
      node.ub = glm::max(node.ub, bounded_meshes[i].aabb.ub_);
    }
    ssize_t count = r - l;
    if (count <= 2) {
      node.primitive_offset = l;
      node.primitive_count = count;
      return;
    }
    AABB center_bounds{};
    for (ssize_t i = l; i < r; ++i)
      center_bounds.Merge(bounded_meshes[i].center);
    auto range = center_bounds.Range();
    int split_axis = 0;
    if (range.y > range.x) split_axis = 1;
    if (range.z > range[split_axis]) split_axis = 2;
    /* Partition */
    ssize_t mid = l + (r - l) / 2;
    std::nth_element(bounded_meshes.begin() + l,
                     bounded_meshes.begin() + mid,
                     bounded_meshes.begin() + r,
                     [=](const BoundedPrimitive& x, const BoundedPrimitive& y) {
                       return x.center[split_axis] < y.center[split_axis];
                     });
    node.child = nodes_.size();
    ssize_t lchild = node.child;
    nodes_.emplace_back();
    nodes_.emplace_back();
    /* Recursion */
    Build(lchild, l, mid, bounded_meshes);
    Build(lchild + 1, mid, r, bounded_meshes);
  }
};

}  // namespace crystal::graphics

#endif