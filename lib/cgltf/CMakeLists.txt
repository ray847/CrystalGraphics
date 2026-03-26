if (NOT TARGET cgltf)
  message("Fetching cgltf...")
  include(FetchContent)
  FetchContent_Declare(
    cgltf_fetch
    GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
    GIT_TAG        v1.14
  )
  FetchContent_MakeAvailable(cgltf_fetch)
  add_library(cgltf INTERFACE)
  target_include_directories(cgltf INTERFACE ${cgltf_fetch_SOURCE_DIR})
  message("cgltf found and exposed.")
endif()