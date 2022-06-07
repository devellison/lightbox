/// \file camera.hpp
/// Camera interface.
#ifndef LIGHTBOX_CAMERA_CAMERA_HPP_
#define LIGHTBOX_CAMERA_CAMERA_HPP_

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
/// TimeStamp used by camera calls (high_resolution_clock)
using TimeStamp = std::chrono::high_resolution_clock::time_point;

/// Get the timestamp right now
/// \return TimeStamp - current time_point on the high resolution clock
TimeStamp TimeStampNow();

/// Frame callback for hosts
/// \param info - information about the camera
/// \param image - CameraFrame containing image.
/// \param timestamp - time the image was taken
typedef std::function<void(const CameraInfo& info, const CameraFrame& image, TimeStamp timestamp)>
    FrameCallback;

/// Camera interface / Base class
/// This may be used as an asynchronous frame source (using the callback)
/// OR as a synchronous one using GetNextFrame() and GetLastFrame().
///
/// All camera types should derive from this and implement the pure functions.
/// Also call OnFrameReceived() when we receive frames.
class Camera
{
 public:
  /// Constructor - requires a CameraInfo from CameraManager's Enumerate() function.
  /// \param info - CameraInfo (returned by CameraManager::Enumerate)
  Camera(const CameraInfo& info);

  /// Virtual dtor
  virtual ~Camera() = default;

  /// Start the frame stream.
  /// @param cb - callback that will be called each frame we receive.
  virtual void Start(FrameCallback cb = nullptr);

  /// Stop the frame stream
  virtual void Stop();

  /// Get the next frame. Waits until a new one comes in.
  /// \param timeout_ms - length of time to wait for a frame
  /// \returns std::optional<CameraFrame> - camera frame or empty on timeout/empty frame.
  virtual std::optional<CameraFrame> GetNewFrame(size_t timeout_ms = 1000);

  /// Gets the last frame (or nothing if no non-empty frames have been read)
  /// \return std::optional<CameraFrame> - last frame or empty if none.
  virtual std::optional<CameraFrame> GetLastFrame();

  /// Is the camera started?
  /// \returns true if Start() has been called.
  bool IsRunning();

  /// Retrieve the camera's info
  /// Note: After creating a camera, the formats member is filled out.
  /// \returns CameraInfo struct containing updated camera info.
  CameraInfo GetCameraInfo();

  /// Sets the camera mode (should be done before calling Start()!)
  /// \param info - Struct from the CameraInfo after creation,
  ///               or build your own and set unimportant members to 0.
  /// Will take the first format that matches non-zero members.
  virtual void SetFormat(const FormatInfo& info);

  /// Retrieves the camera mode. empty if not yet set with SetFormat
  /// \returns std::optional<FormatInfo> - empty if SetFormat not called, otherwise
  ///          contains current format.
  virtual std::optional<FormatInfo> GetFormat();

 protected:
  /// Handles received frame by updating last_frame_ and calling callback if available.
  /// Inheriting classes should call this when they get a frame!
  /// \param frame - frame that has been received.
  virtual void OnFrameReceived(const CameraFrame& frame);

  /// Camera/API specific startup
  /// This should fill out formats in the info_ member and start the camera streaming.
  virtual void OnStart() = 0;

  /// Camera/API specific stop
  /// This should stop the camera streaming, but leave the class in a state where
  /// OnStart() could be called again.
  virtual void OnStop() = 0;

  /// Camera/API specific set camera mode.
  /// \param mode - Requested mode. Fields set to 0 can be ignored.
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