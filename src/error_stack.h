#ifndef CRYSTALGRAPHICS_SRC_ERROR_STACK_H_
#define CRYSTALGRAPHICS_SRC_ERROR_STACK_H_

#include <format>
#include <optional>
#include <stdexcept>

#include "CrystalGraphics/error.h"

namespace crystal::graphics {

class ErrorStack {
 public:
  void Push(Error e) {
    if (last_error_) [[unlikely]]
      throw std::runtime_error(std::format(
          "Unhandled Error:\n{}", *last_error_));
    last_error_ = e;
  }
  std::optional<Error> Pop() {
    auto ret = last_error_;
    last_error_ = {};
    return ret;
  }

 private:
  std::optional<Error> last_error_;
};

} // namespace crystal::graphics

#endif
