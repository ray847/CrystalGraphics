#include <CrystalGraphics/graphics.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <glm/ext/quaternion_geometric.hpp>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

/**
 * Extract the value from a `std::expected` if the value is present, otherwise
 * terminate the program.
 */
template <typename T>
T ExpectedVal(std::expected<T, std::string>&& expected) {
  if (!expected) [[unlikely]] {
    std::cerr << expected.error() << std::endl;
    exit(EXIT_FAILURE);
  }
  if constexpr (!std::is_same_v<T, void>) return std::move(*expected);
}

std::string_view kHelperInfo = R"(
CrystalGraphics Standalone Example
This example will render a glTF file stationary for 5 seconds then start rotating.
Usage: ./main path/to/your/glTF/file
  (-h ; prints this message)
  (-t number of seconds to run;           defaults to 10)
  (-d camera distance to origin;          defaults to 1.0)
  (-o (x, y, z) camera rotation origin;   defaults to (0.0, 0.0, 0.0))
  (--window-width width;                  defaults to 1920)
  (--window-height height;                defaults to 1080)
  (--render-width width;                  defaults to 1280)
  (--render-height height;                defaults to 720)
  (--sample-count count;                  defaults to 1)
  (--trace-depth depth;                   defaults to 2)
  (--max-transport-samples count;         defaults to 3)
  (--max-diffuse-samples count;           defaults to 1)
  (--max-rough-samples count;             defaults to 1)
  (--max-emission-samples count;          defaults to 1)
)";

struct Arguments {
  std::string scene_path;
  double run_seconds = 10.0;
  float camera_distance = 1.0f;
  glm::vec3 rotation_origin{ 0.0f, 0.0f, 0.0f };
  crystal::graphics::PathTraceConf path_trace_conf{};
};

std::optional<Arguments> ParseArguments(int argc,
                                        char** argv,
                                        bool& help_requested);
crystal::graphics::Camera CreateCamera(const Arguments& args, double seconds);

/**
 * Main function.
 */
int main(int argc, char** argv) {
  bool help_requested = false;
  auto args = ParseArguments(argc, argv, help_requested);
  if (!args) return help_requested ? EXIT_SUCCESS : EXIT_FAILURE;

  /* Initialize environment. */
  ExpectedVal(crystal::graphics::EnvInit(args->path_trace_conf));
  auto scene = ExpectedVal(crystal::graphics::LoadScene(args->scene_path));

  /* Run for some seconds. */
  uint64_t counter = 0;  // frame counter
  auto st = std::chrono::high_resolution_clock::now();
  while (std::chrono::high_resolution_clock::now() - st
         < std::chrono::duration<double>{ args->run_seconds }) {
    auto elapsed = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now() - st);
    auto camera = CreateCamera(*args, elapsed.count());
    ExpectedVal(crystal::graphics::EnvPoll());  // poll window
    ExpectedVal(crystal::graphics::View(scene, camera));  // render frame
    ExpectedVal(crystal::graphics::EnvPresent());  // present frame
    counter++;
  }
  std::cout << "Frame Count: " << counter << std::endl;

  /* Terminate environment. */
  ExpectedVal(crystal::graphics::EnvTerm());

  return EXIT_SUCCESS;
}

std::optional<float> ParseFloat(std::string_view value) {
  try {
    size_t read = 0;
    auto result = std::stof(std::string{ value }, &read);
    if (read != value.size()) return std::nullopt;
    return result;
  } catch (...) { return std::nullopt; }
}

std::optional<double> ParseDouble(std::string_view value) {
  try {
    size_t read = 0;
    auto result = std::stod(std::string{ value }, &read);
    if (read != value.size()) return std::nullopt;
    return result;
  } catch (...) { return std::nullopt; }
}

std::optional<std::uint32_t> ParseUInt32(std::string_view value) {
  if (value.empty()) return std::nullopt;
  for (char c : value) {
    if (c < '0' || c > '9') return std::nullopt;
  }

  try {
    size_t read = 0;
    auto result = std::stoul(std::string{ value }, &read);
    if (read != value.size() || result > UINT32_MAX) return std::nullopt;
    return static_cast<std::uint32_t>(result);
  } catch (...) { return std::nullopt; }
}

std::optional<glm::vec3> ParseVec3(std::string value) {
  for (char& c : value) {
    if (c == '(' || c == ')' || c == ',') c = ' ';
  }

  std::stringstream stream{ value };
  glm::vec3 result{};
  if (!(stream >> result.x >> result.y >> result.z)) return std::nullopt;

  std::string trailing;
  if (stream >> trailing) return std::nullopt;
  return result;
}

std::optional<Arguments> ParseArguments(int argc,
                                        char** argv,
                                        bool& help_requested) {
  Arguments args;
  std::vector<std::string> positionals;

  auto parse_uint_option =
      [&](int& i, std::string_view option, std::uint32_t& output) -> bool {
    if (++i >= argc) {
      std::cerr << option << " requires a positive integer.\n";
      return false;
    }
    auto value = ParseUInt32(argv[i]);
    if (!value || *value == 0) {
      std::cerr << option << " must be a positive integer.\n";
      return false;
    }
    output = *value;
    return true;
  };

  for (int i = 1; i < argc; i++) {
    std::string_view arg{ argv[i] };
    if (arg == "-h") {
      std::cout << kHelperInfo;
      help_requested = true;
      return std::nullopt;
    }

    if (arg == "-t") {
      if (++i >= argc) {
        std::cerr << "-t requires a number of seconds.\n";
        return std::nullopt;
      }
      auto seconds = ParseDouble(argv[i]);
      if (!seconds || *seconds <= 0.0) {
        std::cerr << "-t must be a positive number of seconds.\n";
        return std::nullopt;
      }
      args.run_seconds = *seconds;
      continue;
    }

    if (arg == "-d") {
      if (++i >= argc) {
        std::cerr << "-d requires a camera distance.\n";
        return std::nullopt;
      }
      auto distance = ParseFloat(argv[i]);
      if (!distance || *distance <= 0.0f) {
        std::cerr << "-d must be a positive camera distance.\n";
        return std::nullopt;
      }
      args.camera_distance = *distance;
      continue;
    }

    if (arg == "-o") {
      if (i + 1 >= argc) {
        std::cerr << "-o requires a rotation origin.\n";
        return std::nullopt;
      }

      std::string origin{ argv[++i] };
      auto parsed_origin = ParseVec3(origin);
      if (!parsed_origin && i + 2 < argc) {
        origin += " ";
        origin += argv[++i];
        origin += " ";
        origin += argv[++i];
        parsed_origin = ParseVec3(origin);
      }

      if (!parsed_origin) {
        std::cerr << "-o must be formatted like \"(x, y, z)\".\n";
        return std::nullopt;
      }
      args.rotation_origin = *parsed_origin;
      continue;
    }

    if (arg == "--window-width") {
      if (!parse_uint_option(i, arg, args.path_trace_conf.window_width))
        return std::nullopt;
      continue;
    }

    if (arg == "--window-height") {
      if (!parse_uint_option(i, arg, args.path_trace_conf.window_height))
        return std::nullopt;
      continue;
    }

    if (arg == "--render-width") {
      if (!parse_uint_option(i, arg, args.path_trace_conf.render_width))
        return std::nullopt;
      continue;
    }

    if (arg == "--render-height") {
      if (!parse_uint_option(i, arg, args.path_trace_conf.render_height))
        return std::nullopt;
      continue;
    }

    if (arg == "--sample-count") {
      if (!parse_uint_option(i, arg, args.path_trace_conf.sample_count))
        return std::nullopt;
      continue;
    }

    if (arg == "--trace-depth") {
      if (!parse_uint_option(i, arg, args.path_trace_conf.trace_depth))
        return std::nullopt;
      continue;
    }

    if (arg == "--max-transport-samples") {
      if (!parse_uint_option(
              i, arg, args.path_trace_conf.max_transport_sample_count))
        return std::nullopt;
      continue;
    }

    if (arg == "--max-diffuse-samples") {
      if (!parse_uint_option(
              i, arg, args.path_trace_conf.max_diffuse_sample_count))
        return std::nullopt;
      continue;
    }

    if (arg == "--max-rough-samples") {
      if (!parse_uint_option(
              i, arg, args.path_trace_conf.max_rough_sample_count))
        return std::nullopt;
      continue;
    }

    if (arg == "--max-emission-samples") {
      if (!parse_uint_option(
              i, arg, args.path_trace_conf.max_emission_sample_count))
        return std::nullopt;
      continue;
    }

    if (!arg.empty() && arg.front() == '-') {
      std::cerr << "Unknown argument: " << arg << '\n';
      return std::nullopt;
    }

    positionals.emplace_back(arg);
  }

  if (positionals.size() != 1) {
    std::cerr << kHelperInfo;
    return std::nullopt;
  }

  args.scene_path = positionals.front();
  return args;
}

crystal::graphics::Camera CreateCamera(const Arguments& args, double seconds) {
  constexpr double kStationarySeconds = 5.0;
  constexpr double kRotationRadiansPerSecond = 0.5;
  double rotation_seconds = std::max(0.0, seconds - kStationarySeconds);
  float angle =
      static_cast<float>(rotation_seconds * kRotationRadiansPerSecond);

  glm::vec3 offset{ args.camera_distance * std::sin(angle),
                    args.camera_distance * std::cos(angle),
                    0.0f };
  glm::vec3 position = args.rotation_origin + offset;
  glm::vec3 direction = glm::normalize(args.rotation_origin - position);

  return crystal::graphics::Camera{
    .position = position,
    .direction = direction,
    .viewport = { 1.960f / 2.0f, 1.080f / 2.0f },
  };
}
