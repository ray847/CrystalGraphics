if (NOT TARGET CrystalMem)
  message("Fetching CrystalMem...")
  include(FetchContent)
  FetchContent_Declare(
  CrystalMem
  GIT_REPOSITORY https://github.com/ray847/CrystalMem.git
  GIT_TAG "origin/main"
  )
  FetchContent_MakeAvailable(CrystalMem)
  message("CrystalMem found.")
endif()