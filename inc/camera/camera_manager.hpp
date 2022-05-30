#ifndef LIGHTBOX_CAMERA_MANAGER_HPP_
#define LIGHTBOX_CAMERA_MANAGER_HPP_

#include <vector>
#include "camera.hpp"
#include "camera_info.hpp"

namespace zebral
{
/// Enumerates and creates cameras
/// Right now, this is a really simplistic class, and doesn't watch disconnects or usage
class CameraManager
{
 public:
  CameraManager();

  virtual ~CameraManager() = default;

  /// Get a list of camera info structures
  std::vector<CameraInfo> Enumerate();

  /// Create a camera with an information structure from Enumerate
  std::shared_ptr<Camera> Create(const CameraInfo& info);

 protected:
  std::vector<CameraInfo> cameras_;
};

}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_MANAGER_HPP_