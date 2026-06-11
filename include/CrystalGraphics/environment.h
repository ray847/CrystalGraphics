#ifndef CRYSTALGRAPHICS_ENVIRONMENT_H_
#define CRYSTALGRAPHICS_ENVIRONMENT_H_

#include <cstdint>
#include <expected>

#include "error.h"

namespace crystal::graphics {

struct PathTraceConf {
  std::uint32_t window_width = 1920, window_height = 1080;
  std::uint32_t render_width = 1280, render_height = 720;
  std::uint32_t sample_count = 1;
  std::uint32_t trace_depth = 2;
  std::uint32_t max_transport_sample_count = 3, max_diffuse_sample_count = 1,
                max_rough_sample_count = 1, max_emission_sample_count = 1;
};

std::expected<void, Error> EnvInit(PathTraceConf conf = {});
std::expected<void, Error> EnvTerm();
std::expected<void, Error> EnvPoll();
std::expected<void, Error> EnvPresent();
bool EnvStatus();

} // namespace crystal::graphics

#endif
