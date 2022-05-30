/// \file camera_win.hpp
/// Camera for windows using Media Foundation api.
#ifndef LIGHTBOX_CAMERA_CAMERA_WIN_HPP_
#define LIGHTBOX_CAMERA_CAMERA_WIN_HPP_
#ifdef _WIN32

#include "camera.hpp"

namespace zebral
{
/// Windows (Media Foundation) camera implementation
class CameraWin : public Camera
{
 public:
  CameraWin(const CameraInfo& info);
  virtual ~CameraWin();

 protected:
  /// Loops until exiting_ is true grabbing frames
  void CaptureThread();
  /// Creates the capture_ object before thread is started
  void OnStart() override;
  /// Called once the thread stops to clean up the capture_ object
  void OnStop() override;

  /// The implementation sits in camera_win_impl
  /// to avoid throwing all sorts of stuff into the header.
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace zebral

#endif  // _WIN32
#endif  // LIGHTBOX_CAMERA_CAMERA_WIN_HPP_
