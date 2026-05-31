#include <CrystalGraphics/graphics.h>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>

int main() {
  auto init_res = crystal::graphics::EnvInit();
  if (!init_res) {
    std::cerr << init_res.error() << std::endl;
    return EXIT_FAILURE;
  }
  // auto scene = crystal::graphics::LoadScene(
  //     "asset/ABeautifulGame/glTF/ABeautifulGame.gltf");
  auto scene =
      crystal::graphics::LoadScene("asset/FlightHelmet/glTF/FlightHelmet.gltf");
  // auto scene = crystal::graphics::LoadScene("asset/Sponza/glTF/Sponza.gltf");
  // auto scene = crystal::graphics::LoadScene("asset/CompareTransmission/glTF/"
  //                                           "CompareTransmission.gltf");
  if (!scene) {
    std::cout << scene.error() << std::endl;
    return EXIT_FAILURE;
  }
  /* Run for some seconds. */
  uint64_t counter = 0;
  auto st = std::chrono::high_resolution_clock::now();
  while (std::chrono::high_resolution_clock::now() - st
         < std::chrono::seconds(30)) {
    float angle = (float)std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::high_resolution_clock::now() - st)
                      .count()
                / 1000 / 5;
    float dis = 0.5f;
    crystal::graphics::Camera camera{
      .position = { dis * std::cos(angle), dis * std::sin(angle), 0.5f },
      .direction = { -std::cos(angle), -std::sin(angle), 0 },
      .viewport = { 1.960, 1.080 }
    };
    auto view_res = crystal::graphics::View(*scene, camera);
    if (!view_res) [[unlikely]] {
      std::cerr << view_res.error() << std::endl;
      return EXIT_FAILURE;
    }
    auto sync_res = crystal::graphics::EnvSync();
    if (!sync_res) [[unlikely]] {
      std::cerr << sync_res.error() << std::endl;
      return EXIT_FAILURE;
    }
    counter++;
  }
  std::cout << "\rLoop: " << counter << std::endl;
  auto term_res = crystal::graphics::EnvTerm();
  if (!term_res) {
    std::cerr << term_res.error() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
