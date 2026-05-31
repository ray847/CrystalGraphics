#ifndef CRYSTALGRAPHICS_PATHTRACING_SCENE_DATA_H_
#define CRYSTALGRAPHICS_PATHTRACING_SCENE_DATA_H_

#include <filesystem>
#include <vector>

#include "CrystalGraphics/scene.h"
#include "bvh.h"
#include "src/scene_impl.h"

namespace crystal::graphics {

class SceneData {
 public:
  /* Variables */
  const std::filesystem::path file_path_;
  const std::vector<Vertex>& vertices_;
  const std::vector<Material>& materials_;
  const TextureContainer& textures_;
  BVH bvh_;

  /* Constructor */
  SceneData(const Scene& scene) :
      file_path_(scene.FilePath()),
      vertices_(scene.impl_->vertices_),
      materials_(scene.impl_->materials_),
      textures_(scene.impl_->textures_),
      bvh_(scene) {
  }
};

} // namespace crystal::graphics

#endif
