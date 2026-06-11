#ifndef CRYSTALGRAPHICS_API_H_
#define CRYSTALGRAPHICS_API_H_

#include <expected>

#include "camera.h"
#include "error.h"
#include "scene.h"


namespace crystal::graphics {

std::expected<void, Error> View(const Scene& scene, const Camera& camera = {});
// std::expected<void, Error> CPUView(const Scene& scene, const Camera& camera =
// {});

}  // namespace crystal::graphics

#endif