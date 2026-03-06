#include <iostream>
#include <chrono>

#include <CrystalGraphics/graphics.h>

int main() {
  auto init_res = crystal::graphics::EnvInit();
  if (!init_res) {
    std::cerr << init_res.error().msg << std::endl;
    abort();
  }
  /* Run for 5 seconds. */
  auto st = std::chrono::high_resolution_clock::now();
  while (std::chrono::high_resolution_clock::now() - st
         < std::chrono::seconds(3)) {
    auto sync_res = crystal::graphics::EnvSync();
    if (!sync_res) {
      std::cerr << sync_res.error().msg << std::endl;
      abort();
    }
  }
  auto term_res = crystal::graphics::EnvTerm();
  if (!term_res) {
    std::cerr << term_res.error().msg << std::endl;
    abort();
  }
  return 0;
}
