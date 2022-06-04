// To be deprecated
#ifndef _WIN32

/// \file camera_opencv.hpp
/// OpenCV camera - this is really for testing and debugging, should eventually be deprecated and
/// removed.
#ifndef LIGHTBOX_CAMERA_CAMERA_OPENCV_HPP_
#define LIGHTBOX_CAMERA_CAMERA_OPENCV_HPP_
#include "camera.hpp"

namespace zebral
{
/// Simple OpenCV camera implementation
class CameraOpenCV : public Camera
{
 public:
  CameraOpenCV(const CameraInfo& info);
  virtual ~CameraOpenCV();

  static std::vector<CameraInfo> Enumerate();

 protected:
  /// Loops until exiting_ is true grabbing frames
  void CaptureThread();
  /// Creates the capture_ object before thread is started
  void OnStart() override;
  /// Called once the thread stops to clean up the capture_ object
  void OnStop() override;

  class Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace zebral
#endif  // LIGHTBOX_CAMERA_CAMERA_OPENCV_HPP_
#endif  // _WIN32