#include <webgpu/webgpu.hpp>

#include "global.h"
#include "util.h"

namespace crystal::graphics::wgpu {

std::expected<::wgpu::CommandEncoder, Error> CreateCommandEncoder(
    ::wgpu::Device& device) {
  ::wgpu::CommandEncoder encoder =
      device.createCommandEncoder([] -> ::wgpu::CommandEncoderDescriptor {
        ::wgpu::CommandEncoderDescriptor desc{ ::wgpu::Default };
        desc.label = ::wgpu::StringView{ "Crystal Graphics Command Encoder" };
        return desc;
      }());
  if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
  return encoder;
}
std::expected<::wgpu::CommandBuffer, Error> CreateCommandBuffer(
    ::wgpu::CommandEncoder& encoder) {
	::wgpu::CommandBuffer buffer = encoder.finish(
		[] -> ::wgpu::CommandBufferDescriptor {
			::wgpu::CommandBufferDescriptor desc {::wgpu::Default};
			desc.label = ::wgpu::StringView{"Crystal Graphics Command Buffer"};
			return desc;
		}()
	);
	if (auto e = global::error_stack.Pop()) return std::unexpected(*e);
	return buffer;
}
} // namespace crystal::graphics::wgpu