/// \file camera_manager.cpp
/// CameraManager for enumerating and creating cameras

#include "camera_manager.hpp"
#include <functional>
#include <regex>
#include "camera_info.hpp"
#include "camera_platform.hpp"
#include "errors.hpp"
#include "platform.hpp"

namespace zebral
{
CameraManager::CameraManager() {}

std::vector<CameraInfo> CameraManager::Enumerate()
{
  std::vector<CameraInfo> cameras;

  cameras = CameraPlatform::Enumerate();
  return cameras;
}

/// Create a camera with an information structure from Enumerate
std::shared_ptr<Camera> CameraManager::Create(const CameraInfo& info)
{
  return std::shared_ptr<Camera>(new CameraPlatform(info));
}

}  // namespace zebral
