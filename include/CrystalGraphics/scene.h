#ifndef CRYSTALGRAPHICS_SCENE_H_
#define CRYSTALGRAPHICS_SCENE_H_

#include <expected>
#include <filesystem>
#include <memory>

#include "error.h"

namespace crystal::graphics {

class Scene {
 public:
  const auto& FilePath() const {
    return filepath_;
  }
  ~Scene();
  Scene(Scene&& other) noexcept;

 private:
  struct Impl;

  /* Variables */
  const std::filesystem::path filepath_;
  std::unique_ptr<Impl> impl_;

  /* Constructor */
  Scene(const std::filesystem::path& filepath);
  friend std::expected<Scene, Error> LoadScene(std::filesystem::path file);
  friend class SceneData;
  friend class BVH;
  friend class TLAS;
  friend class BLAS;
};

std::expected<Scene, Error> LoadScene(std::filesystem::path file);

} // namespace crystal::graphics

#endif
