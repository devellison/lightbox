/// \file camera.cpp
/// Implementation of camera base class.
#include "camera.hpp"
#include "errors.hpp"
#include "log.hpp"

namespace zebral
{
TimeStamp TimeStampNow()
{
  return std::chrono::high_resolution_clock::now();
}

Camera::Camera(const CameraInfo& info)
    : info_(info),
      callback_(nullptr),
      exiting_(false),
      running_(false),
      decode_(true)
{
}

void Camera::Start(FrameCallback cb)
{
  Stop();
  callback_ = cb;
  OnStart();
  running_ = true;
}

void Camera::Stop()
{
  exiting_ = true;
  OnStop();
  exiting_ = false;
  running_ = false;
}

bool Camera::IsRunning()
{
  return running_;
}

void Camera::OnFrameReceived(const CameraFrame& frame)
{
  TimeStamp timestamp = TimeStampNow();

  // Update last frame and notify anyone waiting
  {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    last_frame_     = frame;
    last_timestamp_ = timestamp;
    cv_.notify_all();
  }

  // Call callback if provided
  if (callback_)
  {
    callback_(info_, frame, timestamp);
  }
}

std::optional<CameraFrame> Camera::GetNewFrame(size_t timeout_ms)
{
  TimeStamp ts = TimeStampNow();
  CameraFrame frame;

  std::unique_lock<std::mutex> lock(frame_mutex_);
  // Wait for a frame to come in - if we get one, stop waiting and return it.
  cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&] {
    if (ts < last_timestamp_)
    {
      frame = last_frame_;
      ts    = last_timestamp_;
      return true;
    }
    return false;
  });

  if (!frame.empty())
  {
    return frame;
  }

  // Timed out
  ZBA_LOG("Timeout or Empty Frame!");
  return {};
}

std::optional<CameraFrame> Camera::GetLastFrame()
{
  std::lock_guard<std::mutex> lock(frame_mutex_);
  if (last_frame_.empty())
  {
    return {};
  }
  return last_frame_;
}

CameraInfo Camera::GetCameraInfo()
{
  std::lock_guard<std::mutex> lock(frame_mutex_);
  return info_;
}

void Camera::SetFormat(const FormatInfo& info, bool decode)
{
  // Find matching format here - if we rely on the inherited classes
  // to do it against the system formats, our sort order won't be
  // taken into account easily.
  for (auto checkFormat : info_.formats)
  {
    if (info.Matches(checkFormat))
    {
      decode_       = decode;
      auto setFmt   = OnSetFormat(checkFormat);
      current_mode_ = std::make_unique<FormatInfo>(setFmt);
      // {TODO} support signed/floats here.
      cur_frame_.reset(setFmt.width, setFmt.height, setFmt.channels, setFmt.bytespppc, false,
                       false);
      ZBA_LOG("Mode for camera {} set. Decode: {}", info_.name, decode_);
      ZBA_LOGSS(*current_mode_.get());
      return;
    }
  }
  ZBA_ERR("No matches for requested format.");
  ZBA_LOGSS(info);
  ZBA_THROW("Format not found!", Result::ZBA_UNSUPPORTED_FMT);
}

/// Retrieves the camera mode. empty if not yet set.
std::optional<FormatInfo> Camera::GetFormat()
{
  if (!current_mode_) return {};
  return *current_mode_.get();
}

bool Camera::IsFormatSupported(const std::string& fourcc)
{
  /// {TODO} These are ones we have reference (SLOW) converters for
  /// so far.  Need to get cameras with other modes....
  /// Like, especially Bayer modes, although most I've used with those
  /// had their own SDKs....
  /// Right now, the only other thing I have is MJPG.
  if (fourcc == "NV12") return true;

#if _WIN32
  if (fourcc == "L8  ") return true;
  if (fourcc == "D16 ") return true;
  if (fourcc == "YUY2") return true;
#else
  if (fourcc == "GREY") return true;
  if (fourcc == "Z16 ") return true;
  if (fourcc == "YUYV") return true;
#endif
  return false;
}

void Camera::AddAllModeEntry(const FormatInfo& format)
{
  all_modes_.emplace_back(format);
}

std::vector<FormatInfo> Camera::GetAllModes()
{
  return all_modes_;
}

/// Convenience operator for dumping CameraInfos for debugging
std::ostream& operator<<(std::ostream& os, const CameraFrame& camFrame)
{
  os << "Frame: " << camFrame.width() << ", " << camFrame.height() << " " << camFrame.channels()
     << " " << camFrame.bytes_per_channel() << std::endl;
  return os;
}

}  // namespace zebral
