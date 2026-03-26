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
add_subdirectory(lib/cgltf)

# Internal Library
add_library(CrystalGraphics)
target_sources(CrystalGraphics
  PRIVATE
    src/graphics.cpp
    src/error.cpp
    src/environment.cpp
    src/camera.cpp
    src/vertex.cpp
    src/mesh.cpp
    src/scene.cpp
    src/api.cpp
    src/view.cpp
    src/error_stack.h
    src/cgltf_implementation.cpp
    src/pathtracing/bvh.cpp
    src/glfw/global.h
    src/glfw/environment.h
    src/glfw/environment.cpp
    src/wgpu/webgpu_implementation.cpp
    src/wgpu/global.h
    src/wgpu/environment.h
    src/wgpu/environment.cpp
    src/wgpu/view.cpp
    src/wgpu/util.h
    src/wgpu/instance.cpp
    src/wgpu/surface.cpp
    src/wgpu/adapter.cpp
    src/wgpu/device.cpp
    src/wgpu/resource.cpp
    src/wgpu/compute.cpp
    src/wgpu/render.cpp
    src/wgpu/queue.cpp
    src/wgpu/command.cpp
)
target_include_directories(CrystalGraphics
  PUBLIC
    # When building the project, use the local include directory
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    # When installed, consumers will look in the standard install include directory
    $<INSTALL_INTERFACE:include>
  PRIVATE
    # Allow internal .cpp files to easily find internal .hpp files in the src/ folder
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(CrystalGraphics
  PUBLIC
    CrystalBase
    CrystalSpatial
    cgltf
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
