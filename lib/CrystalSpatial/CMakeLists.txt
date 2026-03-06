if (NOT TARGET CrystalSpatial)
  message("Fetching CrystalSpatial...")
  include(FetchContent)
  FetchContent_Declare(
  CrystalSpatial
  GIT_REPOSITORY https://github.com/ray847/CrystalSpatial.git
  GIT_TAG "origin/main"
  )
  FetchContent_MakeAvailable(CrystalSpatial)
  message("CrystalSpatial found.")
endif()
