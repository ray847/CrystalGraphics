#include <CrystalGraphics/graphics.h>

#include <chrono>
#include <iostream>

#include "include/CrystalGraphics/scene.h"

int main() {
  auto init_res = crystal::graphics::EnvInit();
  if (!init_res) {
    std::cerr << init_res.error() << std::endl;
    abort();
  }
  /* Run for 3 seconds. */
  crystal::graphics::Scene scene{};
  uint64_t counter = 0;
  auto st = std::chrono::high_resolution_clock::now();
  while (std::chrono::high_resolution_clock::now() - st
         < std::chrono::seconds(1)) {
    auto view_res = crystal::graphics::View(scene);
    std::cout << "\rLoop: " << counter;
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
  auto term_res = crystal::graphics::EnvTerm();
  if (!term_res) {
    std::cerr << term_res.error() << std::endl;
    abort();
  }
  return 0;
}
