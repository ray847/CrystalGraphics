#ifndef CRYSTALGRAPHICS_CAMERA_H_
#define CRYSTALGRAPHICS_CAMERA_H_

#include "CrystalSpatial/primitive/vec3f.h"
#include "CrystalSpatial/primitive/vec2f.h"

namespace crystal::graphics {

struct Camera {
  alignas(16) spatial::vec3f position{0, 0, 0};
  alignas(16) spatial::vec3f direction{1, 0, 0};
  alignas( 8) spatial::vec2f viewport{0.5, 0.5};
};

} // namespace crystal::graphics

#endif