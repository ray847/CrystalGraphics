#ifndef CRYSTALGRAPHICS_CAMERA_H_
#define CRYSTALGRAPHICS_CAMERA_H_

#include "CrystalSpatial/primitive/vec3f.h"
#include "CrystalSpatial/primitive/vec2f.h"

namespace crystal::graphics {

struct Camera {
  spatial::vec3f position{0, 0, 0};
  spatial::vec3f direction{1, 0, 0};
  spatial::vec2f viewport{0.5, 0.5};
};

} // namespace crystal::graphics

#endif