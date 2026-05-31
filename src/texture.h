#ifndef CRYSTALGRAPHICS_SRC_TEXTURE_H_
#define CRYSTALGRAPHICS_SRC_TEXTURE_H_

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace crystal::graphics {

struct TextureData {
  std::string name;
  std::string mime_type;
  std::filesystem::path source_path;
  std::vector<std::byte> encoded_data;
  std::int32_t mag_filter = 0;
  std::int32_t min_filter = 0;
  std::int32_t wrap_s = 0;
  std::int32_t wrap_t = 0;
};

using TextureContainer = std::vector<TextureData>;

struct EnvironmentTextureData {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::vector<float> rgba32f;
};

}  // namespace crystal::graphics

#endif
