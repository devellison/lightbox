/// \file camera_platform.hpp
/// Header for platform-specific cameras
#ifndef LIGHTBOX_CAMERA_CAMERA_PLATFORM_HPP_
#define LIGHTBOX_CAMERA_CAMERA_PLATFORM_HPP_

#include "camera.hpp"
#include "camera_info.hpp"

namespace zebral
{
/// Linux Video4Linux2 camera implementation
class CameraPlatform : public Camera
{
 public:
  CameraPlatform(const CameraInfo& info);
  virtual ~CameraPlatform();

  // Enumerate devices
  static std::vector<CameraInfo> Enumerate();

 protected:
  /// Creates the capture_ object before thread is started
  void OnStart() override;

  /// Called once the thread stops to clean up the capture_ object
  void OnStop() override;

  /// Called to set camera mode. Should throw on failure.
  FormatInfo OnSetFormat(const FormatInfo& info) override;

  /// The implementation sits in the Impl object
  /// to avoid throwing all sorts of stuff into the header.
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace zebral
#endif  // LIGHTBOX_CAMERA_CAMERA_PLATFORM_HPP_
