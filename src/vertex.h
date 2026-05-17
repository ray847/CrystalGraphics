#ifndef CRYSTALGRAPHICS_SRC_VERTEX_H_
#define CRYSTALGRAPHICS_SRC_VERTEX_H_

#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

#include <vector>

namespace crystal::graphics {

struct Vertex {
  alignas(16) glm::vec3 position;
  alignas(16) glm::vec3 normal;
  alignas(8) glm::vec2 tex_coord{ 0.0f, 0.0f };
};

static_assert(alignof(Vertex) == 16);
static_assert(offsetof(Vertex, position) == 0);
static_assert(offsetof(Vertex, normal) == 16);
static_assert(offsetof(Vertex, tex_coord) == 32);
static_assert(sizeof(Vertex) == 48);

using VertexContainer = std::vector<Vertex>;

}  // namespace crystal::graphics

#endif
