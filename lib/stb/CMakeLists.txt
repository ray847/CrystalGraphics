if (NOT TARGET stb_image)
  message("Fetching stb...")
  include(FetchContent)
  FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        31c1ad37456438565541f4919958214b6e762fb4
  )
  FetchContent_MakeAvailable(stb)
  add_library(stb_image INTERFACE)
  target_include_directories(stb_image INTERFACE ${stb_SOURCE_DIR})
  message("stb_image found and exposed.")
endif()
