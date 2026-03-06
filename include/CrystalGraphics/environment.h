#ifndef CRYSTALGRAPHICS_ENVIRONMENT_H_
#define CRYSTALGRAPHICS_ENVIRONMENT_H_

#include <expected>

#include "error.h"

namespace crystal::graphics {

std::expected<void, Error> EnvInit();
std::expected<void, Error> EnvTerm();
std::expected<void, Error> EnvSync();
bool EnvStatus();

} // namespace crystal::graphics

#endif
