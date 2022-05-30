/// \file camera_manager.hpp
/// Camera manager for camera enumeration and creation.

#ifndef LIGHTBOX_CAMERA_MANAGER_HPP_
#define LIGHTBOX_CAMERA_MANAGER_HPP_

#include <vector>
#include "camera.hpp"
#include "camera_info.hpp"

namespace zebral
{
/// Enumerates and creates cameras
///
/// Right now, this is a really simplistic class, and doesn't watch disconnects or usage and just
/// updates the known cameras when Enumerate is called.
///
/// {TODO} Long term, it should watch device connections and allow threaded access.
class CameraManager
{
 public:
  CameraManager();

  virtual ~CameraManager() = default;

  /// Get a list of camera info structures
  std::vector<CameraInfo> Enumerate();

  /// Create a camera with an information structure from Enumerate
  std::shared_ptr<Camera> Create(const CameraInfo& info);
};

}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_MANAGER_HPP_