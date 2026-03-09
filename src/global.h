#ifndef CRYSTALGRAPHICS_SRC_GLOBAL_H_
#define CRYSTALGRAPHICS_SRC_GLOBAL_H_

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu.hpp>
#include <webgpu/webgpu-raii.hpp>
#include <glfw3webgpu.h>

namespace crystal::graphics::global {

inline bool env_status = false;
/* GLFW */
inline GLFWwindow* glfw_window = nullptr;
/* WebGPU */
inline wgpu::raii::Instance wgpu_instance{nullptr};
inline wgpu::raii::Device wgpu_device{nullptr};
inline wgpu::raii::Surface wgpu_surface{nullptr};
inline wgpu::TextureFormat wgpu_surface_format{};
inline wgpu::raii::Queue wgpu_queue{nullptr};
/* Buffers */
inline wgpu::raii::Texture wgpu_surface_texture{nullptr};
inline wgpu::raii::BindGroupLayout wgpu_bindgroup_layout{nullptr};
inline wgpu::raii::RenderPipeline wgpu_render_pipeline{nullptr};

} // namespace crystal::graphics::global

#endif