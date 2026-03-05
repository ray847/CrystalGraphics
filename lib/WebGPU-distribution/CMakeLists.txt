if (NOT TARGET webgpu)
  set(WEBGPU_BACKEND DAWN)
  #set(DAWN_FETCH_DEPENDENCIES ON)
  message("Fetching webgpu...")
  include(FetchContent)
  FetchContent_Declare(
  webgpu
  GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution.git
  GIT_TAG v0.3.0-gamma
  )
  FetchContent_MakeAvailable(webgpu)
  message("webgpu found.")
endif()