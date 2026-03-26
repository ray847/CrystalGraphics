#ifndef CRYSTALGRAPHICS_MESH_H_
#define CRYSTALGRAPHICS_MESH_H_

#include <cstdint>

namespace crystal::graphics {

struct Mesh {
  uint32_t vertex_offset, vertex_count;
  uint32_t index_offset, index_count;
};

}  // namespace crystal::graphics

#endif