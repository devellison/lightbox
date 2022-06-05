#include "camera_manager.hpp"
#include <functional>
#include <regex>
#include "camera_info.hpp"
#include "camera_winrt.hpp"
#if !_WIN32
#include "camera_opencv.hpp"
#endif
#include "errors.hpp"
#include "platform.hpp"

namespace zebral
{
// Remove when we remove OpenCV
// Set kPREF_API to the preferred api for the platform

CameraManager::CameraManager() {}

std::vector<CameraInfo> CameraManager::Enumerate()
{
  std::vector<CameraInfo> cameras;

#if _WIN32
  cameras = CameraWinRt::Enumerate();
#else
  cameras = CameraOpenCV::Enumerate();
#endif
  return cameras;
}

/// Create a camera with an information structure from Enumerate
std::shared_ptr<Camera> CameraManager::Create(const CameraInfo& info)
{
#if _WIN32
  return std::shared_ptr<Camera>(new CameraWinRt(info));
#else
  return std::shared_ptr<Camera>(new CameraOpenCV(info));
#endif
}

}  // namespace zebral