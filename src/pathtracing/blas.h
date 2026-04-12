#ifndef CRYSTALGRAPHICS_SRC_PATHTRACING_BLAS_H_
#define CRYSTALGRAPHICS_SRC_PATHTRACING_BLAS_H_

#include <cstdint>
#include <limits>
#include <ranges>
#include <vector>

#include <glm/ext/vector_float3.hpp>

#include "CrystalGraphics/scene.h"
#include "aabb.h"

namespace crystal::graphics {

struct alignas(32) BLASNode {
  glm::vec3 lb = {
    std::numeric_limits<float>::max(),
    std::numeric_limits<float>::max(),
    std::numeric_limits<float>::max(),
  };
  uint32_t child = 0;
  glm::vec3 ub = {
    std::numeric_limits<float>::min(),
    std::numeric_limits<float>::min(),
    std::numeric_limits<float>::min(),
  };
  uint32_t triangle_count = 0;
};

class BLAS {
 public:
  BLAS(const Scene& scene) {
    nodes_.reserve(scene.vertices_.size() * 2);
    triangles_.reserve(scene.vertices_.size() / 3 * 2);
    for (const auto& primitive : scene.space_.ObjView<Primitive>())
      BuildPrimitive(*primitive, scene);
  }

  const auto& Data() const {
    return nodes_;
  }

 private:
  using Primitive = Scene::Primitive;
  struct BoundedTriangle {
    AABB aabb;
    glm::vec3 center;
  };
  struct TriangleInfo {
    glm::vec3 center;
    ssize_t index;
  };
  struct alignas(16) Triangle {
    ssize_t i0, i1, i2;
  };

  std::vector<BLASNode> nodes_;
  std::vector<Triangle> triangles_;

  void BuildPrimitive(const Primitive& primitive, const Scene& scene) {
    ssize_t triangle_count = primitive.index_count / 3;
    /* Extract triangle info. */
    std::vector<TriangleInfo> triangle_info =
        std::views::iota(0u, triangle_count)
        | std::views::transform([&](ssize_t triangle_idx) -> TriangleInfo {
            ssize_t i0 = scene.indicies_[triangle_idx * 3 + 0];
            ssize_t i1 = scene.indicies_[triangle_idx * 3 + 1];
            ssize_t i2 = scene.indicies_[triangle_idx * 3 + 2];
            const glm::vec3& v0 = scene.vertices_[i0].position;
            const glm::vec3& v1 = scene.vertices_[i1].position;
            const glm::vec3& v2 = scene.vertices_[i2].position;
            return TriangleInfo{ .center = (v0 + v1 + v2) / 3.0f,
                                 .index = triangle_idx };
          })
        | std::ranges::to<std::vector>();
    /* Build tree. */
    nodes_.emplace_back(); // root node
    BuildTree(
        nodes_.size() - 1, 0, triangle_count, triangle_info, primitive, scene);
    /* Extract triangles. */
    for (const auto& info : triangle_info) {
      ssize_t idx_offset = primitive.index_offset + (info.index * 3);
      triangles_.emplace_back(scene.indicies_[idx_offset + 0],
                              scene.indicies_[idx_offset + 1],
                              scene.indicies_[idx_offset + 2]);
    }
  }

  void BuildTree(ssize_t node_idx,
                 ssize_t l,
                 ssize_t r,
                 std::vector<TriangleInfo>& triangle_info,
                 const Primitive& primitive,
                 const Scene& scene) {
    BLASNode& node = nodes_[node_idx];
    /* AABB */
    node.lb = glm::vec3(std::numeric_limits<float>::max());
    node.ub = glm::vec3(std::numeric_limits<float>::lowest());
    for (ssize_t i = l; i < r; ++i) {
      ssize_t idx_offset =
          primitive.index_offset + (triangle_info[i].index * 3);
      ssize_t i0 = scene.indicies_[idx_offset + 0];
      ssize_t i1 = scene.indicies_[idx_offset + 1];
      ssize_t i2 = scene.indicies_[idx_offset + 2];
      const glm::vec3& v0 = scene.vertices_[i0].position;
      const glm::vec3& v1 = scene.vertices_[i1].position;
      const glm::vec3& v2 = scene.vertices_[i2].position;
      node.lb = glm::min(node.lb, glm::min(glm::min(v0, v1), v2));
      node.ub = glm::max(node.ub, glm::max(glm::max(v0, v1), v2));
    }

    /* Leaf */
    ssize_t count = r - l;
    if (count <= 2) {
      node.child = l;
      node.triangle_count = count;
      return;
    }

    /* Median */
    AABB center_bounds{ glm::vec3(std::numeric_limits<float>::max()),
                        glm::vec3(std::numeric_limits<float>::lowest()) };
    for (uint32_t i = l; i < r; ++i)
      center_bounds.Merge(triangle_info[i].center);
    auto range = center_bounds.Range();
    int split_axis = 0;
    if (range.y > range.x) split_axis = 1;
    if (range.z > range[split_axis]) split_axis = 2;

    /* Partition */
    ssize_t mid = l + (r - l) / 2;
    std::nth_element(triangle_info.begin() + l,
                     triangle_info.begin() + mid,
                     triangle_info.begin() + r,
                     [=](const TriangleInfo& a, const TriangleInfo& b) {
                       return a.center[split_axis] < b.center[split_axis];
                     });

    /* Child Nodes */
    node.child = nodes_.size();
    node.triangle_count = 0;
    ssize_t left_child_idx = node.child;
    nodes_.emplace_back();
    nodes_.emplace_back();

    /* Recursion */
    BuildTree(left_child_idx, l, mid, triangle_info, primitive, scene);
    BuildTree(left_child_idx + 1, mid, r, triangle_info, primitive, scene);
  }
};

}

#endif