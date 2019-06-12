#include "util.h"
#include <cstdio>

std::string Util::StringFromFormat(const char* format, ...)
{
  std::va_list ap;
  va_start(ap, format);
  std::string ret = StringFromFormatV(format, ap);
  va_end(ap);
  return ret;
}

std::string Util::StringFromFormatV(const char* format, std::va_list ap)
{
  std::string ret;
#ifdef _MSC_VER
  int len = _vscprintf(format, ap);
  if (len <= 0)
    return {};

  ret.resize(len);
  _vsnprintf_s(ret.data(), len + 1, _TRUNCATE, format, ap);
#else
  int len = vsnprintf(nullptr, 0, format, ap);
  if (len <= 0)
    return {};

  ret.resize(len);
  vsnprintf(ret.data(), len + 1, _TRUNCATE, format, ap);
#endif
  return ret;
}
