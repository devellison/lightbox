/// \file device_v4l2.hpp
/// File descriptor wrapper for a device for V4L2
#ifndef LIGHTBOX_CAMERA_DEVICE_V4L2_HPP_
#define LIGHTBOX_CAMERA_DEVICE_V4L2_HPP_

#include <memory>
#include <string>

namespace zebral
{
/// V4L2 device.  Light wrapper mostly for cleanup
class DeviceV4L2
{
 public:
  static constexpr int InvalidValue = -1;

  /// ctor - accept a handle, closes on destruction
  /// \param handle - handle to manage
  DeviceV4L2(int handle = InvalidValue);

  /// ctor - Opens device from path
  DeviceV4L2(const std::string& path);

  /// move ctor
  DeviceV4L2(DeviceV4L2&&);

  /// Destruct - closes handle if any
  ~DeviceV4L2();

  /// Retrieves the handle
  /// \returns int - the handle
  int get() const;

  /// Retry interrupted ioctls
  int ioctl(long unsigned int request, void* param);

  // Select on the device handle
  int select(float timeout = 10.0f);

  /// Alternative way to get the handle.
  /// \returns int - the handle.
  operator int() const;

  /// Returns true if the handle is valid/open
  /// \returns true if valid handle
  bool valid() const;

  /// Returns true if the handle is not valid
  /// \returns - true if invalid handle
  bool bad() const;

 private:
  int handle_;  ///< Handle from ::open()
};

/// Shared device ptr
typedef std::shared_ptr<DeviceV4L2> DeviceV4L2Ptr;

}  // namespace zebral
#endif  //  LIGHTBOX_CAMERA_DEVICE_V4L2_HPP_