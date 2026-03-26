#include <cmath>
#include <chrono>
#include <iostream>

#include <CrystalGraphics/graphics.h>

#include "include/CrystalGraphics/scene.h"

int main() {
  auto init_res = crystal::graphics::EnvInit();
  if (!init_res) {
    std::cerr << init_res.error() << std::endl;
    abort();
  }
  auto scene =
      crystal::graphics::LoadScene("asset/Box.gltf");
  if (!scene) {
    std::cout << scene.error() << std::endl;
    abort();
  }
  /* Run for some seconds. */
  uint64_t counter = 0;
  auto st = std::chrono::high_resolution_clock::now();
  while (std::chrono::high_resolution_clock::now() - st
         < std::chrono::seconds(1)) {
    float angle = (float)std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::high_resolution_clock::now() - st)
                      .count()
                / 1000;
    float dis = 8.0f;
    crystal::graphics::Camera camera{
      .position = { dis * std::cos(angle), dis * std::sin(angle), 0 },
      .direction = { -std::cos(angle), -std::sin(angle), 0 },
      .viewport = { 1.960, 1.080 }
    };
    auto view_res = crystal::graphics::View(*scene, camera);
    //std::cout << "\rLoop: " << counter;
    if (!view_res) [[unlikely]] {
      std::cerr << view_res.error() << std::endl;
      abort();
    }
    auto sync_res = crystal::graphics::EnvSync();
    if (!sync_res) [[unlikely]] {
      std::cerr << sync_res.error() << std::endl;
      abort();
    }
    counter++;
  }
  std::cout << "\rLoop: " << counter;
  auto term_res = crystal::graphics::EnvTerm();
  if (!term_res) {
    std::cerr << term_res.error() << std::endl;
    abort();
  }
  return 0;
}
