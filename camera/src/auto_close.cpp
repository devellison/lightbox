#if _WIN32
#define _CRT_DECLARE_NONSTDC_NAMES 1  // get unix-style read/write flags
#define _CRT_NONSTDC_NO_DEPRECATE     // c'mon, I just want to call open().
#define _CRT_SECURE_NO_WARNINGS
#endif  // _WIN32

#include "auto_close.hpp"

#include <fcntl.h>
#if __linux__
#include <unistd.h>
#elif _WIN32
#include <io.h>
#endif

namespace zebral
{
AutoClose::AutoClose(int handle) : handle_(handle) {}

AutoClose::~AutoClose()
{
  if (valid())
  {
    ::close(handle_);
  }
}

int AutoClose::get() const
{
  return handle_;
}

void AutoClose::reset(int new_handle)
{
  if (valid())
  {
    ::close(handle_);
  }
  handle_ = new_handle;
}

void AutoClose::clear()
{
  reset();
}

int AutoClose::release()
{
  int value = handle_;
  handle_   = InvalidValue;
  return value;
}

bool AutoClose::valid() const
{
  return (handle_ != InvalidValue);
}

bool AutoClose::bad() const
{
  return !valid();
}
}  // namespace zebral