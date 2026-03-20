#ifndef SRC_WGPU_INSTANCE_H_
#define SRC_WGPU_INSTANCE_H_

#include <expected>

#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "CrystalGraphics/error.h"


namespace crystal::graphics::wgpu {

/* Instance */
std::expected<::wgpu::Instance, Error> CreateInstance();

}  // namespace crystal::graphics::wgpu

#endif