#ifndef CRYSTALGRAPHICS_VERTEX_H_
#define CRYSTALGRAPHICS_VERTEX_H_

#include <glm/ext/vector_float3.hpp>
#include <vector>


namespace crystal::graphics {

struct Vertex {
  alignas(16) glm::vec3 position, normal;
};

using VertexContainer = std::vector<Vertex>;

}  // namespace crystal::graphics

#endif