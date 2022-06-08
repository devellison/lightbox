#include "auto_close.hpp"

#include <fcntl.h>
#include <unistd.h>

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