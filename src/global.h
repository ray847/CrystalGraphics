#ifndef CRYSTALGRAPHICS_SRC_GLOBAL_H_
#define CRYSTALGRAPHICS_SRC_GLOBAL_H_

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu.hpp>
#include <webgpu/webgpu-raii.hpp>
#include <glfw3webgpu.h>

#include "environment.h"

namespace crystal::graphics::global {

inline std::unique_ptr<Env> env = nullptr;

} // namespace crystal::graphics::global

#endif