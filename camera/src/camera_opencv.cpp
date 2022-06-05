#if defined(__linux__)
#include "camera_opencv.hpp"

#include <opencv2/opencv.hpp>

#include "camera2cv.hpp"
#include "errors.hpp"
#include "log.hpp"

#define kPREF_API cv::CAP_V4L2

namespace zebral
{
class CameraOpenCV::Impl
{
 public:
  Impl(CameraOpenCV* parent)
      : parent_(*parent),
        capture_(cv::VideoCapture(parent_.info_.index, parent_.info_.api))
  {
  }

  CameraOpenCV& parent_;
  cv::VideoCapture capture_;   ///< Capture object from OpenCV
  std::thread camera_thread_;  ///< Thread for capture
};

CameraOpenCV::CameraOpenCV(const CameraInfo& info) : Camera(info) {}

CameraOpenCV::~CameraOpenCV() {}

void CameraOpenCV::OnStart()
{
  impl_ = std::make_unique<Impl>(this);
  if (!impl_->capture_.isOpened())
  {
    throw Error("Failed to open camera.", Result::ZBA_CAMERA_OPEN_FAILED);
  }
  impl_->camera_thread_ = std::thread(&CameraOpenCV::CaptureThread, this);
}

void CameraOpenCV::OnStop()
{
  if (!impl_)
  {
    return;
  }
  if (impl_->camera_thread_.joinable())
  {
    impl_->camera_thread_.join();
  }
  impl_.reset();
}

void CameraOpenCV::CaptureThread()
{
  while (!exiting_)
  {
    cv::Mat frame;
    if (impl_->capture_.read(frame))
    {
      if (exiting_)
      {
        return;
      }
      auto image = Converter::CvToCameraFrame(frame);
      OnFrameReceived(image);
    }
  }
}

// This is temporary
// Replace with platform specific code.
std::vector<CameraInfo> CameraOpenCV::Enumerate()
{
  std::vector<CameraInfo> cameras;
  // OpenCV doesn't give a count of devices.
  // Or device names, or really anything useful at all.
  const int kMaxCamSearch = 10;
  for (int i = 0; i < kMaxCamSearch; ++i)
  {
    cv::VideoCapture enumCap;
    if (enumCap.open(i, kPREF_API))
    {
      if (enumCap.isOpened())
      {
        cameras.emplace_back(i, kPREF_API, "", "", nullptr);
      }
      enumCap.release();
    }
  }
  return cameras;
}

}  // namespace zebral
#endif  // __linux__