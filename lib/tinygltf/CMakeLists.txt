if (NOT TARGET tinygltf)
  message("Fetching tinygltf...")
  include(FetchContent)
  FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.9.7
  )
  FetchContent_MakeAvailable(tinygltf)
  message("tinygltf found.")
endif()