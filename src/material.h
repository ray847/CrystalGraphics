#ifndef CRYSTALGRAPHICS_SRC_MATERIAL_H_
#define CRYSTALGRAPHICS_SRC_MATERIAL_H_

#include "CrystalGraphics/public.h"
#include "glm/ext/vector_float3.hpp"

namespace crystal::graphics {

struct Material {
  alignas(16) glm::vec3 base_color;
  float emission_strength;
  alignas(16) glm::vec3 emission_color;
  float roughness;
  float metallic;
  float transmission;
  float ior;
  size32_t flags;
};

} // namespace crystal::graphics

#endif