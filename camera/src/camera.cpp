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

bool Camera::IsFormatSupported(const FormatInfo& fmt)
{
  if (fmt.format == "MJPG") return false;
  return true;
}

}  // namespace zebral
