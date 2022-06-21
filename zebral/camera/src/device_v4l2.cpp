/// \file device_v4l2.cpp
/// Implementation of device object to simplify usage and close handle automatically.

#if __linux__
#include "device_v4l2.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cmath>

#include "errors.hpp"

namespace zebral
{
/// Device open mode
static constexpr auto kDeviceOpenMode = O_RDWR | O_NONBLOCK;

DeviceV4L2::DeviceV4L2(const std::string& path) : handle_(InvalidValue)
{
  handle_ = ::open(path.c_str(), kDeviceOpenMode);
  if (InvalidValue == handle_)
  {
    ZBA_THROW("Failed to open " + path, Result::ZBA_CAMERA_OPEN_FAILED);
  }
}

DeviceV4L2::DeviceV4L2(int handle) : handle_(handle) {}

DeviceV4L2::DeviceV4L2(DeviceV4L2&& src) : handle_(InvalidValue)
{
  std::swap(handle_, src.handle_);
}

DeviceV4L2::~DeviceV4L2()
{
  if (valid())
  {
    ::close(handle_);
    handle_ = InvalidValue;
  }
}

DeviceV4L2::operator int() const
{
  return handle_;
}

int DeviceV4L2::get() const
{
  return handle_;
}

bool DeviceV4L2::valid() const
{
  return (handle_ != InvalidValue);
}

bool DeviceV4L2::bad() const
{
  return !valid();
}

int DeviceV4L2::ioctl(long unsigned int request, void* param)
{
  int result = -1;
  do
  {
    result = ::ioctl(handle_, request, param);
  } while ((-1 == result) && (EINTR == errno));
  return result;
}

int DeviceV4L2::select(float timeout)
{
  // Wait for data to be ready
  fd_set fds;
  struct timeval tv;
  FD_ZERO(&fds);
  FD_SET(handle_, &fds);
  tv.tv_sec  = static_cast<int>(timeout);
  tv.tv_usec = static_cast<int>((timeout - std::round(timeout)) * 1000000);
  return ::select(handle_ + 1, &fds, NULL, NULL, &tv);
}

}  // namespace zebral

#endif  //__linux__