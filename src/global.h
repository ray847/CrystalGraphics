#ifndef CRYSTALGRAPHICS_SRC_GLOBAL_H_
#define CRYSTALGRAPHICS_SRC_GLOBAL_H_

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu.hpp>
#include <webgpu/webgpu-raii.hpp>
#include <glfw3webgpu.h>

namespace crystal::graphics::global {

inline bool env_status = false;

inline GLFWwindow* glfw_window = nullptr;

inline wgpu::raii::Instance wgpu_instance{nullptr};
inline wgpu::raii::Device wgpu_device{nullptr};
inline wgpu::raii::Surface wgpu_surface{nullptr};
inline wgpu::raii::Queue wgpu_queue{nullptr};

} // namespace crystal::graphics::global

#endif