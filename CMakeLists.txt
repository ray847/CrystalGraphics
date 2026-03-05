cmake_minimum_required(VERSION 3.15)

# Project Options
project(CrystalGraphics CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# External Libraries
add_subdirectory(lib/CrystalBase)
add_subdirectory(lib/CrystalMem)
add_subdirectory(lib/glfw)
add_subdirectory(lib/WebGPU-distribution)

# Internal Library
add_library(CrystalGraphics)
target_sources(CrystalGraphics
  PRIVATE
    src/graphics.cpp
)
target_include_directories(CrystalGraphics
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(CrystalGraphics
  PRIVATE
    dawn::webgpu_dawn
)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  message(STATUS "Building CrystalGraphics Standalone (with tests/main)")
  # Main
  add_executable(main main.cpp)
  target_include_directories(main
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(main
  PRIVATE
    CrystalGraphics
  )

  # Testing
  add_subdirectory(test)
endif()
