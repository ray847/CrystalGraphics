#ifndef CRYSTALGRAPHICS_SRC_PATHTRACING_TLAS_H_
#define CRYSTALGRAPHICS_SRC_PATHTRACING_TLAS_H_

#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <limits>
#include <vector>

#include "CrystalGraphics/public.h"
#include "CrystalGraphics/scene.h"
#include "aabb.h"
#include "glm/matrix.hpp"
#include "instance.h"

namespace crystal::graphics {

using Primitive = Scene::Primitive;

struct alignas(16) TLASNode {
  alignas(16) glm::vec3 lb = {
    std::numeric_limits<float>::max(),
    std::numeric_limits<float>::max(),
    std::numeric_limits<float>::max(),
  };
  size32_t child = 0;
  alignas(16) glm::vec3 ub = {
    std::numeric_limits<float>::lowest(),
    std::numeric_limits<float>::lowest(),
    std::numeric_limits<float>::lowest(),
  };
  size32_t instance_idx = 0;
};

class TLAS {
 public:
  TLAS(const Scene& scene, const std::vector<size32_t>& blas_roots) {
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
    for (auto [i, prim_blas_root] : std::views::enumerate(
             std::views::zip(scene.space_.ObjView<Primitive>(), blas_roots))) {
      auto prim = std::get<0>(prim_blas_root);
      size32_t blas_root = std::get<1>(prim_blas_root);
      auto world_trans = prim.SubSpaceIdx().AbsTrans();
      instances_.push_back(Instance{
          .inv_trans = glm::inverse(world_trans.mat),
          .blas_root_idx = blas_root,
          .material_idx = static_cast<size32_t>(i),
      });
      BoundedPrimitive res = {
        .aabb = trans_aabb(AABB{ *prim, scene.vertices_ }, world_trans.mat),
        .prim_idx = static_cast<size32_t>(i),
        .blas_root = blas_roots[static_cast<std::size_t>(i)]
      };
      res.center = res.aabb.Center();
      bounded_prims.push_back(res);
    }
    nodes_.reserve(bounded_prims.size() * 2);
    nodes_.emplace_back();
    Build(0, 0, bounded_prims.size(), bounded_prims, blas_roots);
  }

  const auto& Nodes() const {
    return nodes_;
  }
  const auto& Instances() const {
    return instances_;
  }

 private:
  struct BoundedPrimitive {
    AABB aabb;
    glm::vec3 center;
    size32_t prim_idx;
    size32_t blas_root;
  };

  /* Variables */
  std::vector<TLASNode> nodes_;
  std::vector<Instance> instances_;

  void Build(size32_t node_idx,
             size32_t l,
             size32_t r,
             std::vector<BoundedPrimitive>& bounded_meshes,
             const std::vector<size32_t>& blas_roots) {
    TLASNode& node = nodes_[node_idx];
    for (size32_t i = l; i < r; ++i) {
      node.lb = glm::min(node.lb, bounded_meshes[i].aabb.lb_);
      node.ub = glm::max(node.ub, bounded_meshes[i].aabb.ub_);
    }
    glm::vec3 padding = glm::vec3(0.001f, 0.001f, 0.001f);
    node.lb -= padding;
    node.ub += padding;
    size32_t count = r - l;
    if (count <= 1) {
      node.instance_idx = bounded_meshes[l].prim_idx;
      node.child = 0;
      return;
    }
    AABB center_bounds{};
    for (size32_t i = l; i < r; ++i)
      center_bounds.Merge(bounded_meshes[i].center);
    auto range = center_bounds.Range();
    int split_axis = 0;
    if (range.y > range.x) split_axis = 1;
    if (range.z > range[split_axis]) split_axis = 2;
    /* Partition */
    size32_t mid = l + (r - l) / 2;
    std::nth_element(bounded_meshes.begin() + l,
                     bounded_meshes.begin() + mid,
                     bounded_meshes.begin() + r,
                     [=](const BoundedPrimitive& x, const BoundedPrimitive& y) {
                       return x.center[split_axis] < y.center[split_axis];
                     });
    nodes_[node_idx].child = nodes_.size();
    nodes_[node_idx].instance_idx = 0;
    size32_t lchild = nodes_[node_idx].child;
    nodes_.emplace_back();
    nodes_.emplace_back();
    /* Recursion */
    Build(lchild, l, mid, bounded_meshes, blas_roots);
    Build(lchild + 1, mid, r, bounded_meshes, blas_roots);
  }
};

}  // namespace crystal::graphics

#endif