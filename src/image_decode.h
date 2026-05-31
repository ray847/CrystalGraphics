#ifndef CRYSTALGRAPHICS_SRC_IMAGE_DECODE_H_
#define CRYSTALGRAPHICS_SRC_IMAGE_DECODE_H_

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <vector>

#include "CrystalGraphics/error.h"
#include "texture.h"

namespace crystal::graphics {

struct DecodedImage {
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<std::byte> rgba8;
};

struct DecodedFloatImage {
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<float> rgba32f;
};

std::expected<DecodedImage, Error> DecodeImageRgba8(
    const TextureData& texture);

std::expected<DecodedFloatImage, Error> DecodeImageRgba32f(
    const TextureData& texture);

std::expected<DecodedFloatImage, Error> DecodeImageRgba32f(
    const std::filesystem::path& path);

}  // namespace crystal::graphics

#endif
