#include "global.h"
#include "queue.h"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::Queue, Error> CreateQueue(::wgpu::Device& device) {
  auto queue = device.getQueue();
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return queue;
}

std::expected<void, Error> Sumbit(::wgpu::Queue& queue,
                                  ::wgpu::CommandBuffer& cmd_buffer) {
  queue.submit(cmd_buffer);
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return {};
}

}  // namespace crystal::graphics::wgpu