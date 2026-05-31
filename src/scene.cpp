#include "CrystalGraphics/scene.h"

#include <cgltf.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "CrystalGraphics/public.h"
#include "image_decode.h"
#include "scene_impl.h"

namespace crystal::graphics {

namespace {

int HexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

std::string PercentDecodeString(std::string_view value) {
  std::string res;
  res.reserve(value.size());
  for (std::size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '%' && i + 2 < value.size()) {
      int hi = HexValue(value[i + 1]);
      int lo = HexValue(value[i + 2]);
      if (hi >= 0 && lo >= 0) {
        res.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }
    res.push_back(value[i]);
  }
  return res;
}

std::vector<std::byte> PercentDecodeBytes(std::string_view value) {
  std::vector<std::byte> res;
  res.reserve(value.size());
  for (std::size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '%' && i + 2 < value.size()) {
      int hi = HexValue(value[i + 1]);
      int lo = HexValue(value[i + 2]);
      if (hi >= 0 && lo >= 0) {
        res.push_back(static_cast<std::byte>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }
    res.push_back(static_cast<std::byte>(
        static_cast<unsigned char>(value[i])));
  }
  return res;
}

glm::mat4 LoadNodeTransform(const cgltf_node& node) {
  cgltf_float transform_values[16];
  cgltf_node_transform_local(&node, transform_values);

  glm::mat4 transform(1.0f);
  for (std::size_t column = 0; column < 4; ++column) {
    for (std::size_t row = 0; row < 4; ++row) {
      transform[column][row] =
          static_cast<float>(transform_values[column * 4 + row]);
    }
  }
  return transform;
}

std::expected<std::vector<std::byte>, Error> DecodeBase64(
    std::string_view value) {
  std::vector<std::byte> res;
  int accum = 0;
  int bits = -8;
  auto decode = [](char c) -> int {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
  };

  for (char c : value) {
    if (c == '=') break;
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
    int decoded = decode(c);
    if (decoded < 0) return std::unexpected("Invalid base64 texture data URI.");
    accum = (accum << 6) | decoded;
    bits += 6;
    if (bits >= 0) {
      res.push_back(
          static_cast<std::byte>((accum >> bits) & 0xff));
      bits -= 8;
    }
  }

  return res;
}

std::string MimeTypeFromPath(const std::filesystem::path& path) {
  std::string ext = path.extension().string();
  std::ranges::transform(ext, ext.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  if (ext == ".png") return "image/png";
  if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
  if (ext == ".ktx2") return "image/ktx2";
  if (ext == ".webp") return "image/webp";
  return {};
}

std::expected<std::vector<std::byte>, Error> ReadBinaryFile(
    const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input)
    return std::unexpected("Failed to open texture image: " + path.string());

  std::ifstream::pos_type end = input.tellg();
  if (end < 0)
    return std::unexpected("Failed to measure texture image: " + path.string());

  std::vector<std::byte> res(static_cast<std::size_t>(end));
  input.seekg(0, std::ios::beg);
  if (!res.empty())
    input.read(reinterpret_cast<char*>(res.data()),
               static_cast<std::streamsize>(res.size()));
  if (!input)
    return std::unexpected("Failed to read texture image: " + path.string());
  return res;
}

std::expected<std::vector<std::byte>, Error> LoadDataUri(
    std::string_view uri,
    std::string& mime_type) {
  constexpr std::string_view prefix = "data:";
  std::size_t comma = uri.find(',');
  if (!uri.starts_with(prefix) || comma == std::string_view::npos)
    return std::unexpected("Invalid texture data URI.");

  std::string_view header = uri.substr(prefix.size(), comma - prefix.size());
  std::string_view payload = uri.substr(comma + 1);
  std::size_t semicolon = header.find(';');
  mime_type = std::string(header.substr(0, semicolon));

  bool base64 = header.find(";base64") != std::string_view::npos;
  if (base64) return DecodeBase64(payload);
  return PercentDecodeBytes(payload);
}

std::expected<std::vector<std::byte>, Error> LoadImageBytes(
    const cgltf_image& image,
    const std::filesystem::path& gltf_file,
    std::string& mime_type,
    std::filesystem::path& source_path) {
  if (image.mime_type) mime_type = image.mime_type;

  if (image.buffer_view) {
    const cgltf_buffer_view& view = *image.buffer_view;
    const std::byte* begin = nullptr;
    if (view.data) {
      begin = static_cast<const std::byte*>(view.data);
    } else if (view.buffer && view.buffer->data) {
      begin = static_cast<const std::byte*>(view.buffer->data) + view.offset;
    } else {
      return std::unexpected("Texture buffer view has no loaded data.");
    }

    return std::vector<std::byte>{ begin, begin + view.size };
  }

  if (!image.uri) return std::unexpected("Texture image has no source.");

  std::string_view uri = image.uri;
  if (uri.starts_with("data:")) return LoadDataUri(uri, mime_type);

  std::filesystem::path decoded_path{ PercentDecodeString(uri) };
  source_path = decoded_path.is_absolute() ?
                    decoded_path :
                    gltf_file.parent_path() / decoded_path;
  if (mime_type.empty()) mime_type = MimeTypeFromPath(source_path);
  return ReadBinaryFile(source_path);
}

std::expected<TextureData, Error> LoadTexture(const cgltf_texture& texture,
                                              const std::filesystem::path& file) {
  TextureData res;
  if (texture.name) res.name = texture.name;
  if (texture.sampler) {
    res.mag_filter = texture.sampler->mag_filter;
    res.min_filter = texture.sampler->min_filter;
    res.wrap_s = texture.sampler->wrap_s;
    res.wrap_t = texture.sampler->wrap_t;
  }

  const cgltf_image* image =
      texture.image ? texture.image : texture.basisu_image;
  if (!image) return std::unexpected("Texture has no image source.");

  auto bytes = LoadImageBytes(*image, file, res.mime_type, res.source_path);
  if (!bytes) return std::unexpected(bytes.error());
  res.encoded_data = std::move(*bytes);
  return res;
}

std::expected<void, Error> LoadTextures(const cgltf_data& data,
                                        const std::filesystem::path& file,
                                        TextureContainer& textures) {
  textures.reserve(data.textures_count);
  for (cgltf_size i = 0; i < data.textures_count; ++i) {
    auto texture = LoadTexture(data.textures[i], file);
    if (!texture) return std::unexpected(texture.error());
    textures.push_back(std::move(*texture));
  }
  return {};
}

char DecodeJsonEscape(char c) {
  switch (c) {
    case '"':
    case '\\':
    case '/':
      return c;
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    default:
      return c;
  }
}

std::string JsonUnescape(std::string_view value) {
  std::string res;
  res.reserve(value.size());
  for (std::size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '\\' && i + 1 < value.size()) {
      if (value[i + 1] == 'u' && i + 5 < value.size()) {
        res.push_back('?');
        i += 5;
      } else {
        res.push_back(DecodeJsonEscape(value[i + 1]));
        i += 1;
      }
      continue;
    }
    res.push_back(value[i]);
  }
  return res;
}

std::optional<std::string> FindJsonStringProperty(std::string_view json,
                                                  std::string_view key) {
  std::string quoted_key = "\"" + std::string(key) + "\"";
  std::size_t pos = 0;
  while ((pos = json.find(quoted_key, pos)) != std::string_view::npos) {
    pos += quoted_key.size();
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])))
      ++pos;
    if (pos >= json.size() || json[pos] != ':') continue;
    ++pos;
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])))
      ++pos;
    if (pos >= json.size() || json[pos] != '"') continue;
    ++pos;

    std::size_t end = pos;
    bool escaped = false;
    while (end < json.size()) {
      if (escaped) {
        escaped = false;
      } else if (json[end] == '\\') {
        escaped = true;
      } else if (json[end] == '"') {
        return JsonUnescape(json.substr(pos, end - pos));
      }
      ++end;
    }
  }
  return std::nullopt;
}

std::optional<std::filesystem::path> ResolveEnvironmentPath(
    std::string_view json,
    const std::filesystem::path& gltf_file) {
  constexpr std::array<std::string_view, 6> keys{
    "environment_hdr",
    "environmentHdr",
    "environmentHDR",
    "environmentMap",
    "hdr",
    "hdrFile",
  };
  for (std::string_view key : keys) {
    auto value = FindJsonStringProperty(json, key);
    if (!value || value->empty()) continue;

    std::filesystem::path decoded_path{ PercentDecodeString(*value) };
    if (decoded_path.is_absolute()) return decoded_path;
    return gltf_file.parent_path() / decoded_path;
  }
  return std::nullopt;
}

std::optional<std::filesystem::path> LoadEnvironmentHdrFile(
    const cgltf_data& data,
    const std::filesystem::path& gltf_file) {
  auto read_extras = [&](const cgltf_extras& extras)
      -> std::optional<std::filesystem::path> {
    if (!extras.data) return std::nullopt;
    return ResolveEnvironmentPath(extras.data, gltf_file);
  };
  auto read_extensions = [&](const cgltf_extension* extensions,
                             cgltf_size extension_count)
      -> std::optional<std::filesystem::path> {
    for (cgltf_size i = 0; i < extension_count; ++i) {
      if (!extensions[i].data) continue;
      auto path = ResolveEnvironmentPath(extensions[i].data, gltf_file);
      if (path) return path;
    }
    return std::nullopt;
  };

  if (auto path = read_extras(data.extras)) return path;
  if (auto path = read_extensions(data.data_extensions,
                                  data.data_extensions_count))
    return path;
  if (data.scene) {
    if (auto path = read_extras(data.scene->extras)) return path;
    if (auto path = read_extensions(data.scene->extensions,
                                    data.scene->extensions_count))
      return path;
  }
  return std::nullopt;
}

const cgltf_extension* FindExtension(const cgltf_extension* extensions,
                                     cgltf_size extension_count,
                                     std::string_view name) {
  for (cgltf_size i = 0; i < extension_count; ++i) {
    if (extensions[i].name == nullptr) continue;
    if (name == extensions[i].name) return &extensions[i];
  }
  return nullptr;
}

void SkipJsonWhitespace(std::string_view json, std::size_t& pos) {
  while (pos < json.size()
         && std::isspace(static_cast<unsigned char>(json[pos])))
    ++pos;
}

std::optional<float> ParseJsonFloat(std::string_view json, std::size_t& pos) {
  SkipJsonWhitespace(json, pos);
  std::size_t start = pos;
  if (pos < json.size() && (json[pos] == '-' || json[pos] == '+')) ++pos;
  while (pos < json.size()
         && std::isdigit(static_cast<unsigned char>(json[pos])))
    ++pos;
  if (pos < json.size() && json[pos] == '.') {
    ++pos;
    while (pos < json.size()
           && std::isdigit(static_cast<unsigned char>(json[pos])))
      ++pos;
  }
  if (pos < json.size() && (json[pos] == 'e' || json[pos] == 'E')) {
    ++pos;
    if (pos < json.size() && (json[pos] == '-' || json[pos] == '+')) ++pos;
    while (pos < json.size()
           && std::isdigit(static_cast<unsigned char>(json[pos])))
      ++pos;
  }
  if (start == pos) return std::nullopt;

  return std::stof(std::string(json.substr(start, pos - start)));
}

std::optional<std::array<float, 4>> FindFirstFloat4Property(
    std::string_view json,
    std::string_view property) {
  std::string quoted_property = "\"" + std::string(property) + "\"";
  std::size_t pos = json.find(quoted_property);
  if (pos == std::string_view::npos) return std::nullopt;

  pos = json.find('[', pos + quoted_property.size());
  if (pos == std::string_view::npos) return std::nullopt;
  ++pos;

  std::array<float, 4> values{};
  for (std::size_t i = 0; i < values.size(); ++i) {
    auto value = ParseJsonFloat(json, pos);
    if (!value) return std::nullopt;
    values[i] = *value;

    SkipJsonWhitespace(json, pos);
    if (i + 1 < values.size()) {
      if (pos >= json.size() || json[pos] != ',') return std::nullopt;
      ++pos;
    }
  }
  return values;
}

std::optional<std::array<cgltf_size, 6>> FindFirstSpecularImageSet(
    std::string_view json) {
  std::size_t pos = json.find("\"specularImages\"");
  if (pos == std::string_view::npos) return std::nullopt;

  pos = json.find('[', pos);
  if (pos == std::string_view::npos) return std::nullopt;
  pos = json.find('[', pos + 1);
  if (pos == std::string_view::npos) return std::nullopt;
  ++pos;

  std::array<cgltf_size, 6> image_indices{};
  for (cgltf_size i = 0; i < image_indices.size(); ++i) {
    SkipJsonWhitespace(json, pos);
    if (pos >= json.size() || !std::isdigit(static_cast<unsigned char>(json[pos])))
      return std::nullopt;

    cgltf_size value = 0;
    while (pos < json.size()
           && std::isdigit(static_cast<unsigned char>(json[pos]))) {
      value = value * 10 + static_cast<cgltf_size>(json[pos] - '0');
      ++pos;
    }
    image_indices[i] = value;

    SkipJsonWhitespace(json, pos);
    if (i + 1 < image_indices.size()) {
      if (pos >= json.size() || json[pos] != ',') return std::nullopt;
      ++pos;
    }
  }
  return image_indices;
}

glm::vec3 ImageTexel(const DecodedFloatImage& image, float u, float v) {
  u = std::clamp(u, 0.0f, 1.0f);
  v = std::clamp(v, 0.0f, 1.0f);
  std::uint32_t x =
      std::min(image.width - 1,
               static_cast<std::uint32_t>(u * static_cast<float>(image.width)));
  std::uint32_t y = std::min(
      image.height - 1,
      static_cast<std::uint32_t>(v * static_cast<float>(image.height)));
  std::size_t offset =
      (static_cast<std::size_t>(y) * image.width + x) * 4;
  return glm::vec3{
    image.rgba32f[offset + 0],
    image.rgba32f[offset + 1],
    image.rgba32f[offset + 2],
  };
}

glm::vec3 SampleCube(const std::array<DecodedFloatImage, 6>& faces,
                     glm::vec3 dir) {
  float abs_x = std::abs(dir.x);
  float abs_y = std::abs(dir.y);
  float abs_z = std::abs(dir.z);

  cgltf_size face = 0;
  float u = 0.0f;
  float v = 0.0f;
  if (abs_x >= abs_y && abs_x >= abs_z) {
    if (dir.x >= 0.0f) {
      face = 1;
      u = (-dir.z / abs_x + 1.0f) * 0.5f;
    } else {
      face = 0;
      u = (dir.z / abs_x + 1.0f) * 0.5f;
    }
    v = (-dir.y / abs_x + 1.0f) * 0.5f;
  } else if (abs_y >= abs_z) {
    if (dir.y >= 0.0f) {
      face = 2;
      u = (dir.x / abs_y + 1.0f) * 0.5f;
      v = (dir.z / abs_y + 1.0f) * 0.5f;
    } else {
      face = 3;
      u = (dir.x / abs_y + 1.0f) * 0.5f;
      v = (-dir.z / abs_y + 1.0f) * 0.5f;
    }
  } else {
    if (dir.z >= 0.0f) {
      face = 4;
      u = (dir.x / abs_z + 1.0f) * 0.5f;
      v = (1.0f - dir.y / abs_z) * 0.5f;
    } else {
      face = 5;
      u = (-dir.x / abs_z + 1.0f) * 0.5f;
      v = (1.0f - dir.y / abs_z) * 0.5f;
    }
  }
  return ImageTexel(faces[face], 1.0f - u, 1.0f - v);
}

glm::vec3 EngineToGltfDirection(glm::vec3 dir) {
  return glm::vec3{ dir.x, dir.z, -dir.y };
}

glm::vec3 RotateVector(std::array<float, 4> rotation, glm::vec3 dir) {
  glm::vec3 imaginary{ rotation[0], rotation[1], rotation[2] };
  float real = rotation[3];
  float norm =
      glm::dot(imaginary, imaginary) + real * real;
  if (norm <= 0.0f) return dir;

  imaginary /= std::sqrt(norm);
  real /= std::sqrt(norm);

  glm::vec3 t = 2.0f * glm::cross(imaginary, dir);
  return dir + real * t + glm::cross(imaginary, t);
}

std::array<float, 4> InverseRotation(std::array<float, 4> rotation) {
  return { -rotation[0], -rotation[1], -rotation[2], rotation[3] };
}

EnvironmentTextureData CubeToEnvironmentTexture(
    const std::array<DecodedFloatImage, 6>& faces,
    std::array<float, 4> rotation) {
  constexpr float pi = 3.14159265358979323846f;
  std::uint32_t width = faces[0].width * 4;
  std::uint32_t height = faces[0].height * 2;
  EnvironmentTextureData texture{
    .width = width,
    .height = height,
    .rgba32f = std::vector<float>(static_cast<std::size_t>(width) * height * 4),
  };

  for (std::uint32_t y = 0; y < height; ++y) {
    float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
    float theta = v * pi;
    float sin_theta = std::sin(theta);
    float z = std::cos(theta);
    for (std::uint32_t x = 0; x < width; ++x) {
      float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
      float phi = (u - 0.5f) * 2.0f * pi;
      glm::vec3 engine_dir{ std::cos(phi) * sin_theta,
                            std::sin(phi) * sin_theta,
                            z };
      glm::vec3 gltf_dir = EngineToGltfDirection(engine_dir);
      glm::vec3 image_dir = RotateVector(InverseRotation(rotation), gltf_dir);
      glm::vec3 color = SampleCube(faces, image_dir);
      std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4;
      texture.rgba32f[offset + 0] = color.r;
      texture.rgba32f[offset + 1] = color.g;
      texture.rgba32f[offset + 2] = color.b;
      texture.rgba32f[offset + 3] = 1.0f;
    }
  }
  return texture;
}

std::expected<TextureData, Error> LoadImageAsTextureData(
    const cgltf_image& image,
    const std::filesystem::path& file) {
  TextureData texture;
  if (image.name) texture.name = image.name;
  auto bytes = LoadImageBytes(image, file, texture.mime_type, texture.source_path);
  if (!bytes) return std::unexpected(bytes.error());
  texture.encoded_data = std::move(*bytes);
  return texture;
}

std::expected<std::optional<EnvironmentTextureData>, Error>
LoadImageBasedEnvironmentTexture(const cgltf_data& data,
                                 const std::filesystem::path& file) {
  const cgltf_extension* extension =
      FindExtension(data.data_extensions,
                    data.data_extensions_count,
                    "EXT_lights_image_based");
  if (extension == nullptr || extension->data == nullptr) return std::nullopt;

  auto image_indices = FindFirstSpecularImageSet(extension->data);
  if (!image_indices) return std::nullopt;
  std::array<float, 4> rotation =
      FindFirstFloat4Property(extension->data, "rotation")
          .value_or(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });

  std::array<DecodedFloatImage, 6> faces;
  for (cgltf_size i = 0; i < image_indices->size(); ++i) {
    cgltf_size image_index = (*image_indices)[i];
    if (image_index >= data.images_count)
      return std::unexpected("Image based light references an invalid image.");

    auto texture = LoadImageAsTextureData(data.images[image_index], file);
    if (!texture) return std::unexpected(texture.error());

    auto image = DecodeImageRgba32f(*texture);
    if (!image) return std::unexpected(image.error());
    if (image->width == 0 || image->height == 0)
      return std::unexpected("Image based light has an empty cube face.");
    if (i > 0
        && (image->width != faces[0].width || image->height != faces[0].height))
      return std::unexpected(
          "Image based light cube faces do not share one size.");

    faces[i] = std::move(*image);
  }
  return std::optional<EnvironmentTextureData>{
    CubeToEnvironmentTexture(faces, rotation)
  };
}

MaterialTextureInfo DefaultTextureInfo() {
  return MaterialTextureInfo{
    .index = kInvalidTextureIndex,
    .tex_coord = 0,
    .scale = 1.0f,
    .strength = 1.0f,
  };
}

MaterialTextureInfo LoadTextureInfo(const cgltf_data& data,
                                    const cgltf_texture_view& view) {
  if (!view.texture) return DefaultTextureInfo();
  return MaterialTextureInfo{
    .index = static_cast<std::int32_t>(cgltf_texture_index(&data, view.texture)),
    .tex_coord = static_cast<size32_t>(std::max(view.texcoord, 0)),
    .scale = view.scale,
    .strength = view.scale,
  };
}

MaterialAlphaMode LoadAlphaMode(cgltf_alpha_mode alpha_mode) {
  switch (alpha_mode) {
    case cgltf_alpha_mode_mask:
      return kMaterialAlphaMask;
    case cgltf_alpha_mode_blend:
      return kMaterialAlphaBlend;
    case cgltf_alpha_mode_opaque:
    default:
      return kMaterialAlphaOpaque;
  }
}

Material DefaultMaterial() {
  const MaterialTextureInfo texture_info = DefaultTextureInfo();
  return Material{
    .base_color_factor = { 1.0f, 1.0f, 1.0f, 1.0f },
    .emissive_factor = { 0.0f, 0.0f, 0.0f },
    .emissive_strength = 1.0f,
    .metallic_factor = 1.0f,
    .roughness_factor = 1.0f,
    .alpha_cutoff = 0.5f,
    .alpha_mode = kMaterialAlphaOpaque,
    .ior = 1.5f,
    .dispersion = 0.0f,
    .transmission_factor = 0.0f,
    .thickness_factor = 0.0f,
    .attenuation_color = { 1.0f, 1.0f, 1.0f },
    .attenuation_distance = std::numeric_limits<float>::infinity(),
    .anisotropy_strength = 0.0f,
    .anisotropy_rotation = 0.0f,
    .clearcoat_factor = 0.0f,
    .clearcoat_roughness_factor = 0.0f,
    .iridescence_factor = 0.0f,
    .iridescence_ior = 1.3f,
    .iridescence_thickness_minimum = 100.0f,
    .iridescence_thickness_maximum = 400.0f,
    .sheen_color_factor = { 0.0f, 0.0f, 0.0f },
    .sheen_roughness_factor = 0.0f,
    .specular_color_factor = { 1.0f, 1.0f, 1.0f },
    .specular_factor = 1.0f,
    .flags = 0,
    ._padding0 = 0,
    ._padding1 = 0,
    ._padding2 = 0,
    .base_color_texture = texture_info,
    .metallic_roughness_texture = texture_info,
    .normal_texture = texture_info,
    .occlusion_texture = texture_info,
    .emissive_texture = texture_info,
    .anisotropy_texture = texture_info,
    .clearcoat_texture = texture_info,
    .clearcoat_roughness_texture = texture_info,
    .clearcoat_normal_texture = texture_info,
    .iridescence_texture = texture_info,
    .iridescence_thickness_texture = texture_info,
    .sheen_color_texture = texture_info,
    .sheen_roughness_texture = texture_info,
    .specular_texture = texture_info,
    .specular_color_texture = texture_info,
    .transmission_texture = texture_info,
    .thickness_texture = texture_info,
  };
}

Material LoadMaterial(const cgltf_data& data, const cgltf_material& material) {
  Material res = DefaultMaterial();

  const auto& pbr = material.pbr_metallic_roughness;
  res.base_color_factor = {
    pbr.base_color_factor[0],
    pbr.base_color_factor[1],
    pbr.base_color_factor[2],
    pbr.base_color_factor[3],
  };
  res.metallic_factor = pbr.metallic_factor;
  res.roughness_factor = pbr.roughness_factor;
  res.base_color_texture = LoadTextureInfo(data, pbr.base_color_texture);
  res.metallic_roughness_texture =
      LoadTextureInfo(data, pbr.metallic_roughness_texture);

  res.emissive_factor = {
    material.emissive_factor[0],
    material.emissive_factor[1],
    material.emissive_factor[2],
  };
  if (material.has_emissive_strength)
    res.emissive_strength = material.emissive_strength.emissive_strength;
  res.alpha_mode = LoadAlphaMode(material.alpha_mode);
  res.alpha_cutoff = material.alpha_cutoff;
  if (material.double_sided) res.flags |= kMaterialFlagDoubleSided;
  if (material.unlit) res.flags |= kMaterialFlagUnlit;
  res.normal_texture = LoadTextureInfo(data, material.normal_texture);
  res.occlusion_texture = LoadTextureInfo(data, material.occlusion_texture);
  res.emissive_texture = LoadTextureInfo(data, material.emissive_texture);

  if (material.has_transmission) {
    res.transmission_factor = material.transmission.transmission_factor;
    res.transmission_texture =
        LoadTextureInfo(data, material.transmission.transmission_texture);
  }
  if (material.has_ior) res.ior = material.ior.ior;
  if (material.has_dispersion)
    res.dispersion = material.dispersion.dispersion;
  if (material.has_volume) {
    res.thickness_factor = material.volume.thickness_factor;
    res.attenuation_color = {
      material.volume.attenuation_color[0],
      material.volume.attenuation_color[1],
      material.volume.attenuation_color[2],
    };
    res.attenuation_distance = material.volume.attenuation_distance;
    res.thickness_texture =
        LoadTextureInfo(data, material.volume.thickness_texture);
  }
  if (material.has_clearcoat) {
    res.clearcoat_factor = material.clearcoat.clearcoat_factor;
    res.clearcoat_roughness_factor =
        material.clearcoat.clearcoat_roughness_factor;
    res.clearcoat_texture =
        LoadTextureInfo(data, material.clearcoat.clearcoat_texture);
    res.clearcoat_roughness_texture =
        LoadTextureInfo(data, material.clearcoat.clearcoat_roughness_texture);
    res.clearcoat_normal_texture =
        LoadTextureInfo(data, material.clearcoat.clearcoat_normal_texture);
  }
  if (material.has_iridescence) {
    res.iridescence_factor = material.iridescence.iridescence_factor;
    res.iridescence_ior = material.iridescence.iridescence_ior;
    res.iridescence_thickness_minimum =
        material.iridescence.iridescence_thickness_min;
    res.iridescence_thickness_maximum =
        material.iridescence.iridescence_thickness_max;
    res.iridescence_texture =
        LoadTextureInfo(data, material.iridescence.iridescence_texture);
    res.iridescence_thickness_texture =
        LoadTextureInfo(data, material.iridescence.iridescence_thickness_texture);
  }
  if (material.has_sheen) {
    res.sheen_color_factor = {
      material.sheen.sheen_color_factor[0],
      material.sheen.sheen_color_factor[1],
      material.sheen.sheen_color_factor[2],
    };
    res.sheen_roughness_factor = material.sheen.sheen_roughness_factor;
    res.sheen_color_texture =
        LoadTextureInfo(data, material.sheen.sheen_color_texture);
    res.sheen_roughness_texture =
        LoadTextureInfo(data, material.sheen.sheen_roughness_texture);
  }
  if (material.has_specular) {
    res.specular_color_factor = {
      material.specular.specular_color_factor[0],
      material.specular.specular_color_factor[1],
      material.specular.specular_color_factor[2],
    };
    res.specular_factor = material.specular.specular_factor;
    res.specular_texture =
        LoadTextureInfo(data, material.specular.specular_texture);
    res.specular_color_texture =
        LoadTextureInfo(data, material.specular.specular_color_texture);
  }
  if (material.has_anisotropy) {
    res.anisotropy_strength = material.anisotropy.anisotropy_strength;
    res.anisotropy_rotation = material.anisotropy.anisotropy_rotation;
    res.anisotropy_texture =
        LoadTextureInfo(data, material.anisotropy.anisotropy_texture);
  }

  return res;
}

}  // namespace

Scene::Scene(const std::filesystem::path& filepath) :
    filepath_(filepath), impl_(std::make_unique<Impl>()) {
}

Scene::~Scene() = default;

Scene::Scene(Scene&& other) noexcept :
    filepath_(std::move(other.filepath_)), impl_(std::move(other.impl_)) {
}

std::expected<Scene, Error> LoadScene(std::filesystem::path file) {
  Scene res{ file };
  VertexContainer& vertices = res.impl_->vertices_;
  std::vector<uint32_t>& indices = res.impl_->indices_;
  std::vector<Material>& materials = res.impl_->materials_;
  TextureContainer& textures = res.impl_->textures_;
  spatial::Space<Scene::Impl::SpaceDef>& space = res.impl_->space_;

  /* Open the file. */
  cgltf_options opts{};
  cgltf_data* data = nullptr;
  std::string file_str = file.generic_string();
  if (cgltf_parse_file(&opts, file_str.c_str(), &data) != cgltf_result_success)
    return std::unexpected("cgltf parse file failed.");
  if (cgltf_load_buffers(&opts, data, file_str.c_str())
      != cgltf_result_success) {
    cgltf_free(data);
    return std::unexpected("cgltf load buffers failed.");
  }
  if (cgltf_validate(data) != cgltf_result_success) {
    cgltf_free(data);
    return std::unexpected("cgltf load buffers failed.");
  }

  res.impl_->environment_hdr_file_ = LoadEnvironmentHdrFile(*data, file);
  auto environment_texture = LoadImageBasedEnvironmentTexture(*data, file);
  if (!environment_texture) {
    cgltf_free(data);
    return std::unexpected(environment_texture.error());
  }
  res.impl_->environment_texture_ = std::move(*environment_texture);

  /* Extract textures. */
  auto load_textures_res = LoadTextures(*data, file, textures);
  if (!load_textures_res) {
    cgltf_free(data);
    return std::unexpected(load_textures_res.error());
  }

  /* Extract materials. */
  materials.reserve(data->materials_count + 1);
  materials.push_back(DefaultMaterial());
  for (cgltf_size i = 0; i < data->materials_count; ++i)
    materials.push_back(LoadMaterial(*data, data->materials[i]));

  /* Extrace Meshes & Vertices. */
  std::vector<std::vector<Primitive>> primitives(data->meshes_count);
  for (cgltf_size i = 0; i < data->meshes_count; ++i) {
    const cgltf_mesh& mesh = data->meshes[i];
    /* Record data starting point. */
    uint32_t current_vertex_offset = vertices.size();
    uint32_t current_index_offset = indices.size();
    uint32_t total_vertices_for_mesh = 0;
    uint32_t total_indices_for_mesh = 0;

    for (cgltf_size j = 0; j < mesh.primitives_count; ++j) {
      const cgltf_primitive& primitive = mesh.primitives[j];
      assert(primitive.type == cgltf_primitive_type_triangles
             && "Unrecongnized primitive.");
      /* Vertex Count */
      uint32_t primitive_vertex_count = 0;
      for (cgltf_size k = 0; k < primitive.attributes_count; ++k) {
        if (primitive.attributes[k].type == cgltf_attribute_type_position) {
          primitive_vertex_count = primitive.attributes[k].data->count;
          break;
        }
      }
      uint32_t primitive_vertex_offset = vertices.size();
      vertices.resize(vertices.size() + primitive_vertex_count);
      total_vertices_for_mesh += primitive_vertex_count;

      /* Vertex Data */
      for (cgltf_size k = 0; k < primitive.attributes_count; ++k) {
        const cgltf_attribute& attribute = primitive.attributes[k];
        cgltf_accessor* accessor = attribute.data;
        if (attribute.type == cgltf_attribute_type_position) {
          for (cgltf_size v = 0; v < accessor->count; ++v) {
            cgltf_accessor_read_float(
                accessor,
                v,
                &vertices[primitive_vertex_offset + v].position.x,
                3);
          }
        } else if (attribute.type == cgltf_attribute_type_normal) {
          for (cgltf_size v = 0; v < accessor->count; ++v) {
            cgltf_accessor_read_float(
                accessor,
                v,
                &vertices[primitive_vertex_offset + v].normal.x,
                3);
          }
        } else if (attribute.type == cgltf_attribute_type_texcoord
                   && attribute.index == 0) {
          for (cgltf_size v = 0; v < accessor->count; ++v) {
            cgltf_accessor_read_float(
                accessor,
                v,
                &vertices[primitive_vertex_offset + v].tex_coord0.x,
                2);
          }
        } else if (attribute.type == cgltf_attribute_type_texcoord
                   && attribute.index == 1) {
          for (cgltf_size v = 0; v < accessor->count; ++v) {
            cgltf_accessor_read_float(
                accessor,
                v,
                &vertices[primitive_vertex_offset + v].tex_coord1.x,
                2);
          }
        }
      }

      /* Indices */
      cgltf_accessor* indexAccessor = primitive.indices;
      size32_t primitive_index_offset = indices.size();
      if (indexAccessor) {
        total_indices_for_mesh += indexAccessor->count;
        for (cgltf_size k = 0; k < indexAccessor->count; ++k) {
          uint32_t local_index = cgltf_accessor_read_index(indexAccessor, k);
          indices.push_back(local_index + primitive_vertex_offset);
        }
      }

      size32_t material_idx =
          primitive.material ?
              static_cast<size32_t>(
                  cgltf_material_index(data, primitive.material) + 1) :
              0;

      /* Primitives */
      primitives[i].push_back(
          Primitive{ .vertex_offset = primitive_vertex_offset,
                     .vertex_count = primitive_vertex_count,
                     .index_offset = primitive_index_offset,
                     .index_count = static_cast<size32_t>(
                         primitive.indices ? primitive.indices->count : 0),
                     .material_idx = material_idx });
    }
  }

  /* Extract nodes. */
  auto root_ss = space.RootSubSpace();

  auto extract_node = [&](this auto&& self,
                          const cgltf_node* node,
                          decltype(root_ss) subspace) -> void {
    /* Extract transformation. */
    Scene::Impl::Trans trans;
    trans.SetMatrix(LoadNodeTransform(*node));
    subspace.Trans() = trans;

    if (node->mesh) {
      uint32_t mesh_idx = node->mesh - data->meshes;

      for (const auto& prim : primitives[mesh_idx]) {
        (void)subspace.CreateObj<Primitive>(prim);
      }
    }

    /* Recursion. */
    for (cgltf_size i = 0; i < node->children_count; ++i)
      self(node->children[i], subspace.CreateChild());
  };

  const cgltf_scene* scene = data->scene;
  for (cgltf_size i = 0; i < scene->nodes_count; ++i)
    extract_node(scene->nodes[i], root_ss.CreateChild());

  cgltf_free(data);

  /* Convert glTF's y-up coordinates to z-up. */
  space.RootSubSpace().Trans().IncrRotate(glm::radians(90.0f), { 1, 0, 0 });

  return res;
}

}  // namespace crystal::graphics
