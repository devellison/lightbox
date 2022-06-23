/// \file camera.hpp
/// Camera interface base class
#ifndef LIGHTBOX_CAMERA_CAMERA_HPP_
#define LIGHTBOX_CAMERA_CAMERA_HPP_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <thread>

#include "camera_frame.hpp"
#include "camera_info.hpp"

namespace zebral
{
class Param;

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
  /// \param timeout_ms - length of time to wait for a frame (milliseconds)
  /// \returns std::optional<CameraFrame> - camera frame or empty on timeout/empty frame.
  virtual std::optional<CameraFrame> GetNewFrame(size_t timeout_ms = 5000);

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

  /// Retrieves a list of all modes - even those we don't support -
  /// that are available on a camera.
  virtual std::vector<FormatInfo> GetAllModes();

  /// How do we want the buffers decoded?
  enum class DecodeType
  {
    SYSTEM,    ///< Use system codecs
    INTERNAL,  ///< Use internal decoding
    NONE       ///< Provide raw encoded buffers
  };

  /// Sets the camera mode (should be done before calling Start()!)
  /// {TODO}: Undecoded buffers is a work-in-progress, as is decoding
  /// the buffers ourselves in the library.
  ///
  /// \param info - Struct from the CameraInfo after creation,
  ///               or build your own and set unimportant members to 0.
  /// \param decode - specifies if/how buffers are decoded from their native format.
  ///
  /// Will take the first format that matches non-zero members.
  virtual void SetFormat(const FormatInfo& info, DecodeType decode = DecodeType::INTERNAL);

  /// Retrieves the camera mode. empty if not yet set with SetFormat
  /// \returns std::optional<FormatInfo> - empty if SetFormat not called, otherwise
  ///          contains current format.
  virtual std::optional<FormatInfo> GetFormat();

  virtual std::vector<std::string> GetParameterNames();
  virtual std::shared_ptr<Param> GetParameter(const std::string& name);

 protected:
  /// Checks if we will support a format.
  /// Debating about doing our own codecs or allowing raw passthrough on these,
  /// so there'll be more to this at that point.
  ///
  /// If false is returned, it won't be enumerated in the camera's available
  /// options.
  ///
  /// Right now, just allowing nv12 and yuy2
  virtual bool IsFormatSupported(const std::string& format);

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
  /// \returns FormatInfo - Fully filled out format details of the mode actually set.
  virtual FormatInfo OnSetFormat(const FormatInfo& mode) = 0;

  /// Add mode to ALL modes
  void AddAllModeEntry(const FormatInfo& mode);

  /// Copy a raw buffer into our cur_frame_, making sure that we are allocated
  /// correctly for the raw buffer and not just the decoded buffer.
  /// \param srcPtr - source ptr to data
  /// \param srcStride - width of a line in bytes of source. If 0, assumes unpadded.
  void CopyRawBuffer(const void* srcPtr, int srcStride = 0);

  CameraInfo info_;                           ///< Camera info, used for creation
  std::unique_ptr<FormatInfo> current_mode_;  ///< Current mode, null if unset.
  FrameCallback callback_;                    ///< Optional frame callback
  bool exiting_;                              ///< Exiting flag for capture thread (if any)
  bool running_;                              ///< Running flag - true if camera started
  mutable std::mutex frame_mutex_;            ///< Lock on last_frame
  CameraFrame cur_frame_;                     ///< Last frame received
  CameraFrame last_frame_;                    ///< Last frame received
  TimeStamp last_timestamp_;                  ///< Timestamp of last frame received
  std::condition_variable cv_;                ///< Condition var for frame notification
  DecodeType decode_;                         ///< Specifies if/how buffers are decoded
  std::vector<FormatInfo> all_modes_;         ///< All modes available, even those we don't support
  std::mutex parameter_mutex_;                ///< Protect parameters
  std::map<std::string, std::shared_ptr<Param>> parameters_;
};

/// Convenience operator for dumping CameraInfos for debugging
std::ostream& operator<<(std::ostream& os, const CameraFrame& camFrame);

}  // namespace zebral
#endif  // #define LIGHTBOX_CAMERA_CAMERA_HPP_