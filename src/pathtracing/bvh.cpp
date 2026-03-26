#include <algorithm>
#include <glm/ext/vector_float3.hpp>
#include <ranges>
#include <vector>

#include "CrystalGraphics/mesh.h"
#include "CrystalGraphics/scene.h"
#include "aabb.h"
#include "bvh.h"

namespace crystal::graphics {

BVH::BVH(const Scene& scene) {
  std::vector<BoundedMesh> bounded_meshes;
  bounded_meshes.reserve(scene.space_.ObjView<Mesh>().size());
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
  for (auto [i, mesh] : std::views::enumerate(scene.space_.ObjView<Mesh>())) {
    auto world_trans = mesh.SubSpaceIdx().AbsTrans();
    BoundedMesh res = { .aabb = trans_aabb(AABB{ *mesh, scene.vertices_ },
                                           world_trans.mat),
                        .mesh_idx = static_cast<uint32_t>(i) };
    res.center = res.aabb.Center();
    bounded_meshes.push_back(res);
  }
  tlas_nodes_.reserve(bounded_meshes.size() * 2);
  tlas_nodes_.emplace_back();
  Build(0, 0, bounded_meshes.size(), bounded_meshes);
}

void BVH::Build(uint32_t node_idx,
                uint32_t l,
                uint32_t r,
                std::vector<BoundedMesh>& bounded_meshes) {
  TLASNode& node = tlas_nodes_[node_idx];
  for (uint32_t i = l; i < r; ++i) {
    node.lb = glm::min(node.lb, bounded_meshes[i].aabb.lb_);
    node.ub = glm::max(node.ub, bounded_meshes[i].aabb.ub_);
  }
  uint32_t count = r - l;
  if (count <= 2) {
    node.primitive_offset = l;
    node.primitive_count = count;
    return;
  }
  AABB center_bounds{};
  for (uint32_t i = l; i < r; ++i)
    center_bounds.Merge(bounded_meshes[i].center);
  auto range = center_bounds.Range();
  int split_axis = 0;
  if (range.y > range.x) split_axis = 1;
  if (range.z > range[split_axis]) split_axis = 2;
  /* Partition */
  uint32_t mid = l + (r - l) / 2;
  std::nth_element(bounded_meshes.begin() + l,
                   bounded_meshes.begin() + mid,
                   bounded_meshes.begin() + r,
                   [=](const BoundedMesh& x, const BoundedMesh& y) {
                     return x.center[split_axis] < y.center[split_axis];
                   });
  node.child = tlas_nodes_.size();
  uint32_t lchild = node.child;
  tlas_nodes_.emplace_back();
  tlas_nodes_.emplace_back();
  /* Recursion */
  Build(lchild, l, mid, bounded_meshes);
  Build(lchild + 1, mid, r, bounded_meshes);
}

}  // namespace crystal::graphics