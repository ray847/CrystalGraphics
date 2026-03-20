#ifndef SRC_WGPU_GLOBAL_H_
#define SRC_WGPU_GLOBAL_H_

#include "src/error_stack.h"

namespace crystal::graphics::wgpu::global {

inline ErrorStack error_stack{};

constexpr uint32_t resolution_width = 640;
constexpr uint32_t resolution_height = 480;

}

#endif