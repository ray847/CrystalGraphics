#ifndef SRC_WGPU_LIMITS_H_
#define SRC_WGPU_LIMITS_H_

#include <expected>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"
#include "global.h"

namespace crystal::graphics::wgpu {

/* Instance */
std::expected<::wgpu::Limits, Error> CreateLimits(::wgpu::Adapter& adapter);

}  // namespace crystal::graphics::wgpu

#endif