#pragma once
#include <cstdarg>
#include <string>

namespace Util {
std::string StringFromFormat(const char* format, ...);
std::string StringFromFormatV(const char* format, std::va_list ap);
} // namespace Util