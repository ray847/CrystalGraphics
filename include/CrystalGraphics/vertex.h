#ifndef CRYSTALGRAPHICS_VERTEX_H_
#define CRYSTALGRAPHICS_VERTEX_H_

#include <vector>

#include <glm/ext/vector_float3.hpp>

namespace crystal::graphics {

struct Vertex {
  glm::vec3 position, normal;
};

using VertexContainer = std::vector<Vertex>;

}

#endif