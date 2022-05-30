#include "camera.hpp"
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
      running_(false)
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
  ZEBRAL_LOG("Timeout or Empty Frame!");
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

}  // namespace zebral