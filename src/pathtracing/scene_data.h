#ifndef CRYSTALGRAPHICS_PATHTRACING_SCENE_DATA_H_
#define CRYSTALGRAPHICS_PATHTRACING_SCENE_DATA_H_

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <glm/common.hpp>
#include <glm/ext/vector_float3.hpp>
#include <optional>
#include <utility>
#include <vector>

#include "CrystalGraphics/scene.h"
#include "bvh.h"
#include "src/image_decode.h"
#include "src/scene_impl.h"

namespace crystal::graphics {

class SceneData {
 public:
  /* Variables */
  const std::filesystem::path file_path_;
  const std::vector<Vertex>& vertices_;
  const std::vector<Material>& materials_;
  const TextureContainer& textures_;
  const std::optional<std::filesystem::path>& environment_hdr_file_;
  EnvironmentTextureData environment_texture_;
  BVH bvh_;

  /* Constructor */
  SceneData(const Scene& scene) :
      file_path_(scene.FilePath()),
      vertices_(scene.impl_->vertices_),
      materials_(scene.impl_->materials_),
      textures_(scene.impl_->textures_),
      environment_hdr_file_(scene.impl_->environment_hdr_file_),
      environment_texture_(LoadEnvironmentTexture(
          environment_hdr_file_, scene.impl_->environment_texture_)),
      bvh_(scene) {
  }

 private:
  static EnvironmentTextureData LoadEnvironmentTexture(
      const std::optional<std::filesystem::path>& path,
      const std::optional<EnvironmentTextureData>& image_based_texture) {
    if (image_based_texture) return *image_based_texture;
    if (!path) return FallbackEnvironmentTexture();

    auto image = DecodeImageRgba32f(*path);
    if (!image) return FallbackEnvironmentTexture();

    return EnvironmentTextureData{
      .width = image->width,
      .height = image->height,
      .rgba32f = std::move(image->rgba32f),
    };
  }

  static EnvironmentTextureData FallbackEnvironmentTexture() {
    constexpr std::uint32_t width = 512;
    constexpr std::uint32_t height = 256;
    EnvironmentTextureData texture{
      .width = width,
      .height = height,
      .rgba32f = std::vector<float>(
          static_cast<std::size_t>(width) * height * 4),
    };

    for (std::uint32_t y = 0; y < height; ++y) {
      float t = static_cast<float>(y) / static_cast<float>(height - 1);
      glm::vec3 color = glm::mix(glm::vec3{ 0.7f, 0.8f, 1.0f },
                                 glm::vec3{ 0.05f, 0.05f, 0.06f },
                                 t);
      for (std::uint32_t x = 0; x < width; ++x) {
        std::size_t offset =
            (static_cast<std::size_t>(y) * width + x) * 4;
        texture.rgba32f[offset + 0] = color.r;
        texture.rgba32f[offset + 1] = color.g;
        texture.rgba32f[offset + 2] = color.b;
        texture.rgba32f[offset + 3] = 1.0f;
      }
    }
    return texture;
  }
};

} // namespace crystal::graphics

#endif
