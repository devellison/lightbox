/// \file camera_manager.hpp
/// Camera manager for camera enumeration and creation.

#ifndef LIGHTBOX_CAMERA_MANAGER_HPP_
#define LIGHTBOX_CAMERA_MANAGER_HPP_

#include <memory>
#include <vector>

namespace zebral
{
struct CameraInfo;
class Camera;

/// Enumerates and creates cameras
///
/// Right now, this is a really simplistic class, and doesn't watch disconnects or usage and just
/// updates the known cameras when Enumerate is called.
///
/// {TODO} Long term, it should watch device connections, allow threaded access,
///        maybe support config serialization and similar.
class CameraManager
{
 public:
  CameraManager();
  virtual ~CameraManager() = default;

  /// Get a list of camera info structures
  /// These can be passed to Create().
  /// \return std::vector<CameraInfo> - list of cameras.
  std::vector<CameraInfo> Enumerate();

  /// Create a camera with an information structure from Enumerate
  /// \param info - Info about the camera to create.
  /// \returns - std::shared_ptr<Camera> - pointer to created camera, or nullptr on failure.
  std::shared_ptr<Camera> Create(const CameraInfo& info);
};

}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_MANAGER_HPP_