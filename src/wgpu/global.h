#ifndef SRC_WGPU_GLOBAL_H_
#define SRC_WGPU_GLOBAL_H_

#include <cstdint>

#include "src/error_stack.h"

namespace crystal::graphics::wgpu::global {

inline ErrorStack error_stack{};

constexpr std::uint32_t kTraceStackSize = 64;

constexpr char kResolutionWidthOverrideId[] = "0";
constexpr char kResolutionHeightOverrideId[] = "1";
constexpr char kRenderSampleCountOverrideId[] = "2";
constexpr char kLodMaxDepthOverrideId[] = "3";
constexpr char kTraceDepthOverrideId[] = "4";
constexpr char kMaxTransportSampleCountOverrideId[] = "5";
constexpr char kMaxEmissionSampleCountOverrideId[] = "6";
constexpr char kMaxDiffuseSampleCountOverrideId[] = "7";
constexpr char kMaxRoughSampleCountOverrideId[] = "8";

}  // namespace crystal::graphics::wgpu::global

#endif
