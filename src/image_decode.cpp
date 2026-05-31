#include "image_decode.h"

#include <cstring>
#include <limits>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace crystal::graphics {

std::expected<DecodedImage, Error> DecodeImageRgba8(
    const TextureData& texture) {
  if (texture.encoded_data.empty())
    return std::unexpected("Texture has no encoded image data.");
  if (texture.encoded_data.size()
      > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    return std::unexpected("Texture image data is too large to decode.");

  int width = 0;
  int height = 0;
  int channel_count = 0;
  stbi_uc* pixels = stbi_load_from_memory(
      reinterpret_cast<const stbi_uc*>(texture.encoded_data.data()),
      static_cast<int>(texture.encoded_data.size()),
      &width,
      &height,
      &channel_count,
      STBI_rgb_alpha);
  if (pixels == nullptr) {
    std::string message = "Failed to decode texture image";
    if (const char* reason = stbi_failure_reason(); reason != nullptr) {
      message += ": ";
      message += reason;
    }
    message += ".";
    return std::unexpected(message);
  }

  DecodedImage res;
  res.width = static_cast<uint32_t>(width);
  res.height = static_cast<uint32_t>(height);
  std::size_t byte_size =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
  res.rgba8.resize(byte_size);
  std::memcpy(res.rgba8.data(), pixels, byte_size);
  stbi_image_free(pixels);
  return res;
}

std::expected<DecodedFloatImage, Error> DecodeImageRgba32f(
    const TextureData& texture) {
  if (texture.encoded_data.empty())
    return std::unexpected("Texture has no encoded image data.");
  if (texture.encoded_data.size()
      > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    return std::unexpected("Texture image data is too large to decode.");

  int width = 0;
  int height = 0;
  int channel_count = 0;
  float* pixels = stbi_loadf_from_memory(
      reinterpret_cast<const stbi_uc*>(texture.encoded_data.data()),
      static_cast<int>(texture.encoded_data.size()),
      &width,
      &height,
      &channel_count,
      4);
  if (pixels == nullptr) {
    std::string message = "Failed to decode texture image as float";
    if (const char* reason = stbi_failure_reason(); reason != nullptr) {
      message += ": ";
      message += reason;
    }
    message += ".";
    return std::unexpected(message);
  }

  DecodedFloatImage res;
  res.width = static_cast<uint32_t>(width);
  res.height = static_cast<uint32_t>(height);
  std::size_t float_count =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
  res.rgba32f.resize(float_count);
  std::memcpy(res.rgba32f.data(), pixels, float_count * sizeof(float));
  stbi_image_free(pixels);
  return res;
}

std::expected<DecodedFloatImage, Error> DecodeImageRgba32f(
    const std::filesystem::path& path) {
  int width = 0;
  int height = 0;
  int channel_count = 0;
  float* pixels =
      stbi_loadf(path.string().c_str(), &width, &height, &channel_count, 4);
  if (pixels == nullptr) {
    std::string message = "Failed to decode HDR image: ";
    message += path.string();
    if (const char* reason = stbi_failure_reason(); reason != nullptr) {
      message += ": ";
      message += reason;
    }
    message += ".";
    return std::unexpected(message);
  }

  DecodedFloatImage res;
  res.width = static_cast<uint32_t>(width);
  res.height = static_cast<uint32_t>(height);
  std::size_t float_count =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
  res.rgba32f.resize(float_count);
  std::memcpy(res.rgba32f.data(), pixels, float_count * sizeof(float));
  stbi_image_free(pixels);
  return res;
}

}  // namespace crystal::graphics
