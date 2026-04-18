#ifndef CRYSTALGRAPHICS_SRC_PATHTRACING_BLAS_H_
#define CRYSTALGRAPHICS_SRC_PATHTRACING_BLAS_H_

#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <limits>
#include <ranges>
#include <vector>

#include "CrystalGraphics/scene.h"
#include "aabb.h"

namespace crystal::graphics {

struct alignas(16) BLASNode {
  alignas(16) glm::vec3 lb = {
    std::numeric_limits<float>::max(),
    std::numeric_limits<float>::max(),
    std::numeric_limits<float>::max(),
  };
  uint32_t child = 0;
  alignas(16) glm::vec3 ub = {
    std::numeric_limits<float>::min(),
    std::numeric_limits<float>::min(),
    std::numeric_limits<float>::min(),
  };
  uint32_t triangle_count = 0;
};

struct alignas(16) Index {
  size32_t i0, i1, i2;
};

class BLAS {
 public:
  BLAS(const Scene& scene) {
    roots_.reserve(scene.space_.ObjView<Primitive>().size());
    nodes_.reserve(scene.indicies_.size() / 3 * 2);
    indices_.reserve(scene.indicies_.size() / 3);
    for (const auto& primitive : scene.space_.ObjView<Primitive>())
      BuildPrimitive(*primitive, scene);
  }

  const auto& Nodes() const {
    return nodes_;
  }

  const auto& Roots() const {
    return roots_;
  }

  const auto& Indices() const {
    return indices_;
  }

 private:
  using Primitive = Scene::Primitive;
  struct BoundedTriangle {
    AABB aabb;
    glm::vec3 center;
  };
  struct TriangleInfo {
    glm::vec3 center;
    size32_t index;
  };

  /* Variables */
  std::vector<size32_t> roots_;
  std::vector<BLASNode> nodes_;
  std::vector<Index> indices_;

  void BuildPrimitive(const Primitive& primitive, const Scene& scene) {
    size32_t triangle_count = primitive.index_count / 3;
    /* Extract triangle info. */
    std::vector<TriangleInfo> triangle_info =
        std::views::iota(0u, triangle_count)
        | std::views::transform([&](size32_t triangle_idx) -> TriangleInfo {
            size32_t i0 =
                scene.indicies_[primitive.index_offset + triangle_idx * 3 + 0];
            size32_t i1 =
                scene.indicies_[primitive.index_offset + triangle_idx * 3 + 1];
            size32_t i2 =
                scene.indicies_[primitive.index_offset + triangle_idx * 3 + 2];
            const glm::vec3& v0 = scene.vertices_[i0].position;
            const glm::vec3& v1 = scene.vertices_[i1].position;
            const glm::vec3& v2 = scene.vertices_[i2].position;
            return TriangleInfo{ .center = (v0 + v1 + v2) / 3.0f,
                                 .index = triangle_idx };
          })
        | std::ranges::to<std::vector>();
    /* Build tree. */
    roots_.emplace_back(nodes_.size()); // root node
    nodes_.emplace_back();
    // Record the global offset before building the tree
    size32_t global_triangle_offset = indices_.size();
    BuildTree(nodes_.size() - 1,
              0,
              triangle_count,
              triangle_info,
              primitive,
              scene,
              global_triangle_offset);
    /* Extract triangles. */
    for (const auto& info : triangle_info) {
      size32_t idx_offset = primitive.index_offset + (info.index * 3);
      indices_.emplace_back(scene.indicies_[idx_offset + 0],
                            scene.indicies_[idx_offset + 1],
                            scene.indicies_[idx_offset + 2]);
    }
  }

  void BuildTree(size32_t node_idx,
                 size32_t l,
                 size32_t r,
                 std::vector<TriangleInfo>& triangle_info,
                 const Primitive& primitive,
                 const Scene& scene,
                 size32_t triangle_offset) {
    BLASNode& node = nodes_[node_idx];
    /* AABB */
    node.lb = glm::vec3(std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max());
    node.ub = glm::vec3(std::numeric_limits<float>::lowest(),
                        std::numeric_limits<float>::lowest(),
                        std::numeric_limits<float>::lowest());
    for (size32_t i = l; i < r; ++i) {
      size32_t idx_offset =
          primitive.index_offset + (triangle_info[i].index * 3);
      size32_t i0 = scene.indicies_[idx_offset + 0];
      size32_t i1 = scene.indicies_[idx_offset + 1];
      size32_t i2 = scene.indicies_[idx_offset + 2];
      const glm::vec3& v0 = scene.vertices_[i0].position;
      const glm::vec3& v1 = scene.vertices_[i1].position;
      const glm::vec3& v2 = scene.vertices_[i2].position;
      node.lb = glm::min(node.lb, glm::min(glm::min(v0, v1), v2));
      node.ub = glm::max(node.ub, glm::max(glm::max(v0, v1), v2));
    }
    glm::vec3 padding = glm::vec3(0.001f, 0.001f, 0.001f);
    node.lb -= padding;
    node.ub += padding;

    /* Leaf */
    size32_t count = r - l;
    if (count <= 2) {
      node.child = triangle_offset + l;
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
    size32_t mid = l + (r - l) / 2;
    std::nth_element(triangle_info.begin() + l,
                     triangle_info.begin() + mid,
                     triangle_info.begin() + r,
                     [=](const TriangleInfo& a, const TriangleInfo& b) {
                       return a.center[split_axis] < b.center[split_axis];
                     });

    /* Child Nodes */
    nodes_[node_idx].child = nodes_.size();
    nodes_[node_idx].triangle_count = 0;
    size32_t left_child_idx = nodes_[node_idx].child;
    nodes_.emplace_back();
    nodes_.emplace_back();

    /* Recursion */
    BuildTree(left_child_idx,
              l,
              mid,
              triangle_info,
              primitive,
              scene,
              triangle_offset);
    BuildTree(left_child_idx + 1,
              mid,
              r,
              triangle_info,
              primitive,
              scene,
              triangle_offset);
  }
};

}  // namespace crystal::graphics

#endif