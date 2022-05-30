#include "camera_manager.hpp"
#include "camera_opencv.hpp"
#include "platform.hpp"

namespace zebral
{
CameraManager::CameraManager() {}

/// Get a list of camera info structures
std::vector<CameraInfo> CameraManager::Enumerate()
{
  // This is temporary - OpenCV sucks for dealing directly with cameras.
  // Replace with platform specific code.
  cameras_.clear();

  const int kMaxCamSearch = 5;

  for (int i = 0; i < kMaxCamSearch; ++i)
  {
    cv::VideoCapture enumCap;
    if (enumCap.open(i, kPreferredVideoApi))
    {
      cameras_.emplace_back(i, kPreferredVideoApi);
      enumCap.release();
    }
  }
  return cameras_;
}

/// Create a camera with an information structure from Enumerate
std::shared_ptr<Camera> CameraManager::Create(const CameraInfo& /*info*/)
{
  return nullptr;
  // return std::shared_ptr<Camera>(new CameraOpenCV(info));
}

}  // namespace zebral