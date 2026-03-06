#ifndef CRYSTALGRAPHICS_API_H_
#define CRYSTALGRAPHICS_API_H_

#include <expected>

#include "scene.h"
#include "error.h"

namespace crystal::graphics {

std::expected<void, Error> View(const Scene& scene);

} // namespace crystal::graphics

#endif