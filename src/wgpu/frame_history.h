#ifndef CRYSTALGRAPHICS_SRC_WGPU_FRAME_HISTORY_H_
#define CRYSTALGRAPHICS_SRC_WGPU_FRAME_HISTORY_H_

#include <cstdint>

#include "CrystalGraphics/camera.h"

namespace crystal::graphics::wgpu {

class FrameHistory {
 public:
  std::uint32_t NextIteration(std::uint64_t scene_tag, const Camera& camera) {
    if (!has_previous_ || previous_scene_tag_ != scene_tag
        || !SameCamera(previous_camera_, camera)) {
      has_previous_ = true;
      previous_scene_tag_ = scene_tag;
      previous_camera_ = camera;
      frame_count_ = 1;
      return 0;
    }

    return frame_count_++;
  }

  std::uint64_t PreviousSceneTag() const {
    return previous_scene_tag_;
  }

  const Camera& PreviousCamera() const {
    return previous_camera_;
  }

  std::uint32_t FrameCount() const {
    return frame_count_;
  }

 private:
  static bool SameCamera(const Camera& lhs, const Camera& rhs) {
    return lhs.position.x == rhs.position.x
        && lhs.position.y == rhs.position.y
        && lhs.position.z == rhs.position.z
        && lhs.direction.x == rhs.direction.x
        && lhs.direction.y == rhs.direction.y
        && lhs.direction.z == rhs.direction.z
        && lhs.viewport.x == rhs.viewport.x
        && lhs.viewport.y == rhs.viewport.y;
  }

  bool has_previous_ = false;
  std::uint64_t previous_scene_tag_ = 0;
  Camera previous_camera_{};
  std::uint32_t frame_count_ = 0;
};

}  // namespace crystal::graphics::wgpu

#endif
