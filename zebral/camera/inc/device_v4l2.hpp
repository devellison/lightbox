/// \file device_v4l2.hpp
/// File descriptor wrapper for a device for V4L2
#ifndef LIGHTBOX_CAMERA_DEVICE_V4L2_HPP_
#define LIGHTBOX_CAMERA_DEVICE_V4L2_HPP_

#include <linux/videodev2.h>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include "log.hpp"

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

  /// Retrieves a value from a video control
  /// via VIDIOC_G_CTRL.
  /// \param id - v4l2 control id
  /// \param value - receives value on success, casted
  ///                from int to T.
  /// \returns int - ioctl error, 0 on success.
  template <class T>
  int get_video_ctrl(int id, T& value)
  {
    v4l2_control control;
    std::memset(&control, 0, sizeof(control));
    control.id = id;
    auto res   = ioctl(VIDIOC_G_CTRL, &control);
    if (0 == res)
    {
      value = static_cast<T>(control.value);
    }
    return res;
  }

  /// Sets a value to a video control
  /// via VIDIOC_G_CTRL.
  /// \param id - v4l2 control id
  /// \param value - value to set (after casting to int)
  /// \returns int - ioctl error, 0 on success.
  template <class T>
  int set_video_ctrl(int id, const T value)
  {
    v4l2_control control;
    std::memset(&control, 0, sizeof(control));
    control.id    = id;
    control.value = static_cast<int>(std::round(value));
    int result    = ioctl(VIDIOC_S_CTRL, &control);
    ZBA_LOG("Set control {:x} to {}", control.id, control.value);
    return result;
  }

  /// Starts video capture stream on device
  int start_video_stream();

  /// Stops video stream on device
  int stop_video_stream();

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