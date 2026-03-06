#ifndef CRYSTALGRAPHICS_ERROR_H_
#define CRYSTALGRAPHICS_ERROR_H_

#include <stacktrace>
#include <string>

namespace crystal::graphics  {

struct Error {
  std::string msg;
  std::stacktrace stacktrace = std::stacktrace::current();
};

} // namespace crystal::graphics

#endif
