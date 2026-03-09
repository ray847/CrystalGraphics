cmake_minimum_required(VERSION 3.15)

# Project Options
project(CrystalGraphics CXX C)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# External Libraries
add_subdirectory(lib/CrystalBase)
add_subdirectory(lib/CrystalSpatial)
add_subdirectory(lib/glfw)
add_subdirectory(lib/WebGPU-distribution)
add_subdirectory(lib/glfw3webgpu)

# Internal Library
add_library(CrystalGraphics)
target_sources(CrystalGraphics
  PRIVATE
    src/graphics.cpp
    src/error.cpp
    src/environment.cpp
    src/scene.cpp
    src/api.cpp
    src/view.cpp
    src/webgpu_implementation.cpp
    src/error_stack.h
)
target_include_directories(CrystalGraphics
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(CrystalGraphics
  PUBLIC
    CrystalBase
  PRIVATE
    glfw
    webgpu
    glfw3webgpu
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
  target_copy_webgpu_binaries(main)

  # Testing
  add_subdirectory(test)
endif()
