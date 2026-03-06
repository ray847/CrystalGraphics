if (NOT TARGET glfw3webgpu)
  message("Fetching glfw3webgpu...")
  include(FetchContent)
  FetchContent_Declare(
  glfw3webgpu
  GIT_REPOSITORY https://github.com/eliemichel/glfw3webgpu.git
  GIT_TAG v1.3.0-alpha
  )
  FetchContent_MakeAvailable(glfw3webgpu)
  message("glfw3webgpu found.")
endif()
