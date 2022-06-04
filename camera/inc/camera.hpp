/// \file camera.hpp
/// Camera interface.
#ifndef LIGHTBOX_CAMERA_CAMERA_HPP_
#define LIGHTBOX_CAMERA_CAMERA_HPP_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include "camera_frame.hpp"
#include "camera_info.hpp"

namespace zebral
{
/// Generic timestamp
using TimeStamp = std::chrono::high_resolution_clock::time_point;

/// Get the timestamp right now
TimeStamp TimeStampNow();

/// Frame callback
typedef std::function<void(const CameraInfo& info, const CameraFrame& image, TimeStamp timestamp)>
    FrameCallback;

/// Base Camera interface
/// This may be used as an asynchronous frame source (using the callback)
/// OR as a synchronous one using GetNextFrame() and GetLastFrame().
class Camera
{
 public:
  Camera(const CameraInfo& info);
  virtual ~Camera() = default;

  /// Start the frame stream.
  /// @param cb - callback that will be called each frame we receive.
  virtual void Start(FrameCallback cb = nullptr);

  /// Stop the frame stream
  virtual void Stop();

  /// Get the next frame. Waits until a new one comes in.
  virtual std::optional<CameraFrame> GetNewFrame(size_t timeout_ms = 1000);

  /// Gets the last frame (or nothing if no non-empty frames have been read)
  virtual std::optional<CameraFrame> GetLastFrame();

  /// Is the camera started?
  bool IsRunning();

  /// Retrieve the camera's info
  CameraInfo GetCameraInfo();

  /// Sets the camera mode (should be done before calling Start()!)
  virtual void SetFormat(const FormatInfo& info);

  /// Retrieves the camera mode. empty if not yet set.
  virtual std::optional<FormatInfo> GetFormat();

 protected:
  /// Handles received frame by updating last_frame_ and calling callback if available.
  virtual void OnFrameReceived(const CameraFrame& frame);

  /// Camera/API specific startup
  virtual void OnStart() = 0;

  /// Camera/API specific stop
  virtual void OnStop() = 0;

  /// Camera/API specific set camera mode.
  virtual void OnSetFormat(const FormatInfo& mode) = 0;

  CameraInfo info_;                           ///< Camera info, used for creation
  std::unique_ptr<FormatInfo> current_mode_;  ///< Current mode, null if unset.
  FrameCallback callback_;                    ///< Optional frame callback
  bool exiting_;                              ///< Exiting flag for capture thread (if any)
  bool running_;                              ///< Running flag - true if camera started
  mutable std::mutex frame_mutex_;            ///< Lock on last_frame
  CameraFrame last_frame_;                    ///< Last frame received
  TimeStamp last_timestamp_;                  ///< Timestamp of last frame received
  std::condition_variable cv_;                ///< Condition var for frame notification
};

}  // namespace zebral
#endif  // #define LIGHTBOX_CAMERA_CAMERA_HPP_