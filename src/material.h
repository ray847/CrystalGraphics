#ifndef CRYSTALGRAPHICS_SRC_MATERIAL_H_
#define CRYSTALGRAPHICS_SRC_MATERIAL_H_

#include "CrystalGraphics/public.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"

#include <cstdint>

namespace crystal::graphics {

inline constexpr std::int32_t kInvalidTextureIndex = -1;

enum MaterialAlphaMode : size32_t {
  kMaterialAlphaOpaque = 0,
  kMaterialAlphaMask = 1,
  kMaterialAlphaBlend = 2,
};

inline constexpr size32_t kMaterialFlagDoubleSided = 1u << 0u;
inline constexpr size32_t kMaterialFlagUnlit = 1u << 1u;

struct MaterialTextureInfo {
  std::int32_t index;
  size32_t tex_coord;
  float scale;
  float strength;
};

struct Material {
  alignas(16) glm::vec4 base_color_factor;
  alignas(16) glm::vec3 emissive_factor;
  float emissive_strength;

  float metallic_factor;
  float roughness_factor;
  float alpha_cutoff;
  MaterialAlphaMode alpha_mode;

  float ior;
  float dispersion;
  float transmission_factor;
  float thickness_factor;

  alignas(16) glm::vec3 attenuation_color;
  float attenuation_distance;

  float anisotropy_strength;
  float anisotropy_rotation;
  float clearcoat_factor;
  float clearcoat_roughness_factor;

  float iridescence_factor;
  float iridescence_ior;
  float iridescence_thickness_minimum;
  float iridescence_thickness_maximum;

  alignas(16) glm::vec3 sheen_color_factor;
  float sheen_roughness_factor;

  alignas(16) glm::vec3 specular_color_factor;
  float specular_factor;

  size32_t flags;
  size32_t _padding0;
  size32_t _padding1;
  size32_t _padding2;

  MaterialTextureInfo base_color_texture;
  MaterialTextureInfo metallic_roughness_texture;
  MaterialTextureInfo normal_texture;
  MaterialTextureInfo occlusion_texture;
  MaterialTextureInfo emissive_texture;

  MaterialTextureInfo anisotropy_texture;
  MaterialTextureInfo clearcoat_texture;
  MaterialTextureInfo clearcoat_roughness_texture;
  MaterialTextureInfo clearcoat_normal_texture;
  MaterialTextureInfo iridescence_texture;
  MaterialTextureInfo iridescence_thickness_texture;
  MaterialTextureInfo sheen_color_texture;
  MaterialTextureInfo sheen_roughness_texture;
  MaterialTextureInfo specular_texture;
  MaterialTextureInfo specular_color_texture;
  MaterialTextureInfo transmission_texture;
  MaterialTextureInfo thickness_texture;
};

static_assert(sizeof(MaterialTextureInfo) == 16);
static_assert(alignof(Material) == 16);

} // namespace crystal::graphics

#endif
