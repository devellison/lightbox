/// \file camera_v4l2.hpp
/// Camera for linux using Video4Linux2
#if __linux__
#ifndef LIGHTBOX_CAMERA_CAMERA_V4L2_HPP_
#define LIGHTBOX_CAMERA_CAMERA_V4L2_HPP_

#include "camera.hpp"
#include "camera_info.hpp"

namespace zebral
{
/// Linux Video4Linux2 camera implementation
class CameraV4L2 : public Camera
{
 public:
  CameraV4L2(const CameraInfo& info);
  virtual ~CameraV4L2();

  // Enumerate V4L2 devices
  static std::vector<CameraInfo> Enumerate();

 protected:
  /// Takes a device path string, returns vid/pid if found.
  /// \param busPath - Bus path
  /// \param vid - receives USB vendorId on success
  /// \param pid - receives USB pid on success
  /// \return true if vid/pid found.
  static bool VidPidFromBusPath(const std::string& busPath, uint16_t& vid, uint16_t& pid);

  /// Loops until exiting_ is true grabbing frames
  void CaptureThread();

  /// Creates the capture_ object before thread is started
  void OnStart() override;

  /// Called once the thread stops to clean up the capture_ object
  void OnStop() override;

  /// Called to set camera mode. Should throw on failure.
  FormatInfo OnSetFormat(const FormatInfo& info) override;

  /// The implementation sits in camera_win_impl
  /// to avoid throwing all sorts of stuff into the header.
  class Impl;
  std::unique_ptr<Impl> impl_;
};
/// Make CameraPlatform refer to CameraV4L2
typedef CameraV4L2 CameraPlatform;
}  // namespace zebral
#endif  // LIGHTBOX_CAMERA_CAMERA_V4L2_HPP_
#endif  // __linux__
