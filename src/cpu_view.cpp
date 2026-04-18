#include <algorithm>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <stack>
#include <vector>

#include "CrystalGraphics/api.h"
#include "pathtracing/scene_data.h"

namespace crystal::graphics {

struct Ray {
  glm::vec3 pos;
  glm::vec3 dir;
};

struct RayHitInfo {
  float val;
  bool hit_blas;
};

bool RayIntersectAABB(const Ray& ray,
                      const glm::vec3& lb,
                      const glm::vec3& ub) {
  float tmin = 0.0f;
  float tmax = 1e10f;

  for (int i = 0; i < 3; ++i) {
    float invD = 1.0f / ray.dir[i];
    float t0 = (lb[i] - ray.pos[i]) * invD;
    float t1 = (ub[i] - ray.pos[i]) * invD;
    if (invD < 0.0f) { std::swap(t0, t1); }
    tmin = std::max(tmin, t0);
    tmax = std::min(tmax, t1);
    if (tmax <= tmin) { return false; }
  }
  return true;
}

float RayTraverseBLAS(const Ray& ray,
                      uint32_t root_idx,
                      const std::vector<BLASNode>& blas_nodes) {
  uint32_t stack[32];
  int stack_top = 0;
  stack[0] = root_idx;
  float nodes_visited = 0.0f;

  while (stack_top >= 0) {
    uint32_t node_idx = stack[stack_top--];
    const auto& node = blas_nodes[node_idx];

    if (RayIntersectAABB(ray, node.lb, node.ub)) {
      nodes_visited += 1.0f;

      if (node.triangle_count == 0) {
                // Interior node
        stack[++stack_top] = node.child;
        stack[++stack_top] = node.child + 1;
      } else {
                // Leaf node
        nodes_visited += (float)node.triangle_count;
      }
    }
  }
  return nodes_visited;
}

RayHitInfo RayTraverseTLAS(const Ray& ray, const SceneData& scene_data) {
  uint32_t stack[32];
  int stack_top = 0;
  stack[0] = 0;
  float nodes_visited = 0.0f;
  bool hit_blas = false;

  const auto& tlas_nodes = scene_data.bvh_.TLAS().Nodes();
  const auto& instances = scene_data.bvh_.TLAS().Instances();
  const auto& blas_nodes = scene_data.bvh_.BLAS().Nodes();

  while (stack_top >= 0) {
    uint32_t node_idx = stack[stack_top--];
    const auto& node = tlas_nodes[node_idx];

    if (RayIntersectAABB(ray, node.lb, node.ub)) {
      nodes_visited += 1.0f;

      if (node.child != 0) {
                // Interior node
        stack[++stack_top] = node.child;
        stack[++stack_top] = node.child + 1;
      } else {
                // Leaf node
        hit_blas = true;
        // Mimicking the shader's behavior exactly (including the early return
        // if present) However, the shader code provided had a return before the
        // BLAS traversal. Let's look at the shader again. It says: hit_blas =
        // true; return RayHitInfo(nodes_visited, hit_blas);
        // // LEAF TLAS Node!
        // ... dive into BLAS ...

        // If I mimic the shader EXACTLY as written in the previous read:
        return { nodes_visited, hit_blas };
      }
    }
  }
  return { nodes_visited, hit_blas };
}

Ray CameraRay(const glm::vec2& coord, const Camera& camera) {
  glm::vec3 dx =
      glm::normalize(glm::cross(glm::vec3(0, 1, 0), camera.direction))
      * camera.viewport.x;
  glm::vec3 dy =
      glm::normalize(glm::cross(dx, camera.direction)) * camera.viewport.y;
  return Ray{ camera.position,
              glm::normalize(camera.direction + coord.x * dx + coord.y * dy) };
}

glm::vec3 RayColor(const Ray& ray, const SceneData& scene_data) {
  RayHitInfo hit_info = RayTraverseTLAS(ray, scene_data);
  float brightness = glm::clamp(hit_info.val / 20.0f, 0.0f, 1.0f);
  if (hit_info.hit_blas) {
    return glm::vec3(brightness, 0.0f, 0.0f);
  } else {
    return glm::vec3(0.0f, brightness, 0.0f);
  }
}

std::expected<void, Error> CPUView(const Scene& scene, const Camera& camera) {
  SceneData scene_data(scene);
  const int width = 800;
  const int height = 600;
  std::vector<glm::vec3> framebuffer(width * height);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      glm::vec2 pixel(x, y);
      glm::vec2 dims(width, height);
      glm::vec2 coord = (pixel / dims) * 2.0f - glm::vec2(1.0f, 1.0f);

      Ray ray = CameraRay(coord, camera);
      framebuffer[y * width + x] = RayColor(ray, scene_data);
    }
  }

  std::ofstream ofs("output.ppm");
  ofs << "P3\n" << width << " " << height << "\n255\n";
  for (int i = 0; i < width * height; ++i) {
    for (int j = 0; j < 3; ++j) {
      ofs << glm::clamp<int>(framebuffer[i][j] * 255, 0, 255) << ' ';
    }
    ofs << '\n';
  }
  ofs.close();

  return {};
}

}  // namespace crystal::graphics
