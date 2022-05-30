/// \file camera_v4l2.hpp
/// Camera for linux using Video4Linux2

#ifndef LIGHTBOX_CAMERA_CAMERA_V4L2_HPP_
#define LIGHTBOX_CAMERA_CAMERA_V4L2_HPP_
#ifdef __linux__

#include "camera.hpp"

namespace zebral
{
/// Linux Video4Linux2 camera implementation
class CameraV4L2 : public Camera
{
 public:
  CameraV4L2(const CameraInfo& info);
  virtual ~CameraV4L2();

 protected:
  /// Loops until exiting_ is true grabbing frames
  void CaptureThread();
  /// Creates the capture_ object before thread is started
  void OnStart() override;
  /// Called once the thread stops to clean up the capture_ object
  void OnStop() override;
};

}  // namespace zebral

#endif  // __linux__
#endif  // LIGHTBOX_CAMERA_CAMERA_V4L2_HPP_
