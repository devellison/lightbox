#if _WIN32
#ifndef LIGHTBOX_CAMERA_CAMERA_WINRT_HPP_
#define LIGHTBOX_CAMERA_CAMERA_WINRT_HPP_

#include "camera.hpp"
#include "camera_info.hpp"

namespace zebral
{
/// Windows RT camera implementation
class CameraWinRt : public Camera
{
 public:
  CameraWinRt(const CameraInfo& info);
  virtual ~CameraWinRt();

  // Enumerate WinRT devices
  static std::vector<CameraInfo> Enumerate();

 protected:
  /// Takes a device path string, returns vid/pid if found.
  /// \param devPath - Windows DevicePath string
  /// \param vid - receives USB vendorId on success
  /// \param pid - receives USB pid on success
  /// \return true if vid/pid found.
  static bool VidPidFromPath(const std::string& devPath, uint16_t& vid, uint16_t& pid);

  /// Loops until exiting_ is true grabbing frames
  void CaptureThread();

  /// Creates the capture_ object before thread is started
  void OnStart() override;

  /// Called once the thread stops to clean up the capture_ object
  void OnStop() override;

  /// Called to set camera mode. Should throw on failure.
  void OnSetFormat(const FormatInfo& info);

  /// The implementation sits in camera_win_impl
  /// to avoid throwing all sorts of stuff into the header.
  class Impl;
  std::unique_ptr<Impl> impl_;
};
/// Make CameraPlatform refer to CameraWinRt
typedef CameraWinRt CameraPlatform;
}  // namespace zebral
#endif  // LIGHTBOX_CAMERA_CAMERA_WINRT_HPP_
#endif  // _WIN32
