#ifndef CRYSTALGRAPHICS_PATHTRACING_EMISSIVE_BUFFER_H_
#define CRYSTALGRAPHICS_PATHTRACING_EMISSIVE_BUFFER_H_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <optional>
#include <utility>
#include <vector>

#include "CrystalGraphics/public.h"
#include "CrystalGraphics/scene.h"
#include "src/image_decode.h"
#include "src/material.h"
#include "src/scene_impl.h"
#include "src/vertex.h"

namespace crystal::graphics {

struct alignas(16) EmissiveVertex {
  glm::vec3 position;
  float _pad = 0.0f;
};

struct alignas(16) EmissivePrim {
  std::array<EmissiveVertex, 3> vertices;
  alignas(16) glm::vec3 emission;
  float area = 0.0f;
  float weight = 0.0f;
  float _pad0 = 0.0f;
  float _pad1 = 0.0f;
  float _pad2 = 0.0f;
};

class EmissiveBuffer {
 public:
  EmissiveBuffer(const Scene& scene) {
    const Scene::Impl& impl = *scene.impl_;
    std::vector<std::optional<DecodedImage>> decoded_textures(
        impl.textures_.size());
    for (const auto& primitive_ref : impl.space_.ObjView<Primitive>()) {
      const Primitive& primitive = *primitive_ref;
      if (primitive.material_idx >= impl.materials_.size()) {
        continue;
      }

      const Material& material = impl.materials_[primitive.material_idx];
      const glm::vec3 base_emission = Emission(material);
      if (Luminance(base_emission) <= 0.0f) {
        continue;
      }

      const glm::mat4 world_transform =
          primitive_ref.SubSpaceIdx().AbsTrans().mat;
      for (size32_t i = 0; i + 2 < primitive.index_count; i += 3) {
        const size32_t idx_offset = primitive.index_offset + i;
        const size32_t i0 = impl.indices_[idx_offset + 0];
        const size32_t i1 = impl.indices_[idx_offset + 1];
        const size32_t i2 = impl.indices_[idx_offset + 2];
        const Vertex& v0 = impl.vertices_[i0];
        const Vertex& v1 = impl.vertices_[i1];
        const Vertex& v2 = impl.vertices_[i2];
        const glm::vec3 p0 = TransformPoint(world_transform, v0.position);
        const glm::vec3 p1 = TransformPoint(world_transform, v1.position);
        const glm::vec3 p2 = TransformPoint(world_transform, v2.position);
        const float area = TriangleArea(p0, p1, p2);
        if (area <= 0.0f) continue;

        const glm::vec3 emission =
            base_emission
            * EmissiveTextureFactor(
                material, v0, v1, v2, impl.textures_, decoded_textures);
        const float emission_weight = Luminance(emission);
        if (emission_weight <= 0.0f) continue;

        data_.push_back(EmissivePrim{
            .vertices = {
              EmissiveVertex{ .position = p0 },
              EmissiveVertex{ .position = p1 },
              EmissiveVertex{ .position = p2 },
            },
            .emission = emission,
            .area = area,
            .weight = area * emission_weight,
        });
        total_weight_ += data_.back().weight;
      }
    }

    if (total_weight_ > 0.0f) {
      for (EmissivePrim& prim : data_) prim.weight /= total_weight_;
    }
  }

  const std::vector<EmissivePrim>& Data() const {
    return data_;
  }

  float TotalWeight() const {
    return total_weight_;
  }

 private:
  /* Fields */
  std::vector<EmissivePrim> data_;
  float total_weight_ = 0.0f;

  static glm::vec3 Emission(const Material& material) {
    return glm::max(material.emissive_factor * material.emissive_strength,
                    glm::vec3(0.0f));
  }

  static float Luminance(const glm::vec3& color) {
    return color.r * 0.2126f + color.g * 0.7152f + color.b * 0.0722f;
  }

  static glm::vec3 TransformPoint(const glm::mat4& transform,
                                  const glm::vec3& point) {
    return glm::vec3(transform * glm::vec4(point, 1.0f));
  }

  static float TriangleArea(const glm::vec3& p0,
                            const glm::vec3& p1,
                            const glm::vec3& p2) {
    return glm::length(glm::cross(p1 - p0, p2 - p0)) * 0.5f;
  }

  static glm::vec3 EmissiveTextureFactor(
      const Material& material,
      const Vertex& v0,
      const Vertex& v1,
      const Vertex& v2,
      const TextureContainer& textures,
      std::vector<std::optional<DecodedImage>>& decoded_textures) {
    const MaterialTextureInfo& texture_info = material.emissive_texture;
    if (texture_info.index < 0) return glm::vec3(1.0f);

    const std::size_t texture_idx =
        static_cast<std::size_t>(texture_info.index);
    if (texture_idx >= textures.size()) return glm::vec3(1.0f);

    if (!decoded_textures[texture_idx]) {
      auto decoded = DecodeImageRgba8(textures[texture_idx]);
      if (!decoded) return glm::vec3(1.0f);
      decoded_textures[texture_idx] = std::move(*decoded);
    }

    const glm::vec2 uv = TriangleUv(texture_info.tex_coord, v0, v1, v2);
    return SampleTexture(
        *decoded_textures[texture_idx], textures[texture_idx], uv);
  }

  static glm::vec2 TriangleUv(size32_t tex_coord,
                              const Vertex& v0,
                              const Vertex& v1,
                              const Vertex& v2) {
    if (tex_coord == 1) {
      return (v0.tex_coord1 + v1.tex_coord1 + v2.tex_coord1) / 3.0f;
    }
    return (v0.tex_coord0 + v1.tex_coord0 + v2.tex_coord0) / 3.0f;
  }

  static glm::vec3 SampleTexture(const DecodedImage& image,
                                 const TextureData& texture,
                                 glm::vec2 uv) {
    uv.x = WrapCoord(uv.x, texture.wrap_s);
    uv.y = WrapCoord(uv.y, texture.wrap_t);

    const std::uint32_t x = std::min(
        image.width - 1,
        static_cast<std::uint32_t>(uv.x * static_cast<float>(image.width)));
    const std::uint32_t y = std::min(
        image.height - 1,
        static_cast<std::uint32_t>(uv.y * static_cast<float>(image.height)));
    const std::size_t offset =
        (static_cast<std::size_t>(y) * image.width + x) * 4;
    return glm::vec3{
      ByteToFloat(image.rgba8[offset + 0]),
      ByteToFloat(image.rgba8[offset + 1]),
      ByteToFloat(image.rgba8[offset + 2]),
    };
  }

  static float WrapCoord(float coord, std::int32_t wrap) {
    constexpr std::int32_t kClampToEdge = 33071;
    constexpr std::int32_t kMirroredRepeat = 33648;

    if (wrap == kClampToEdge) return std::clamp(coord, 0.0f, 0.999999f);
    if (wrap == kMirroredRepeat) {
      float wrapped = coord - std::floor(coord);
      const auto tile = static_cast<std::int32_t>(std::floor(coord));
      if (tile % 2 != 0) wrapped = 1.0f - wrapped;
      return std::clamp(wrapped, 0.0f, 0.999999f);
    }
    float wrapped = coord - std::floor(coord);
    return std::clamp(wrapped, 0.0f, 0.999999f);
  }

  static float ByteToFloat(std::byte value) {
    return static_cast<float>(std::to_integer<unsigned int>(value)) / 255.0f;
  }
};

static_assert(sizeof(EmissiveVertex) == 16);
static_assert(alignof(EmissiveVertex) == 16);
static_assert(sizeof(EmissivePrim) == 80);
static_assert(alignof(EmissivePrim) == 16);

} // namespace crystal::graphics

#endif
