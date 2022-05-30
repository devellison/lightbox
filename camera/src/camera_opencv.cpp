#include "camera_opencv.hpp"
#include "errors.hpp"
#include "log.hpp"

namespace zebral
{
CameraOpenCV::CameraOpenCV(const CameraInfo& info) : Camera(info) {}

CameraOpenCV::~CameraOpenCV() {}

void CameraOpenCV::OnStart()
{
  capture_ = std::make_unique<cv::VideoCapture>(cv::VideoCapture(info_.index, info_.api));
  if (!capture_->isOpened())
  {
    capture_.reset();
    throw Error("Failed to open camera.", Result::ZBA_CAMERA_OPEN_FAILED);
  }
  camera_thread_ = std::thread(&CameraOpenCV::CaptureThread, this);
}

void CameraOpenCV::OnStop()
{
  if (camera_thread_.joinable())
  {
    camera_thread_.join();

    capture_->release();
    capture_.reset();
  }
}

void CameraOpenCV::CaptureThread()
{
  while (!exiting_)
  {
    cv::Mat frame;
    if (capture_->read(frame))
    {
      if (exiting_)
      {
        return;
      }

      if (!frame.empty())
      {
        bool is_signed = true;
        bool is_float  = false;
        auto depth     = frame.type() & CV_MAT_DEPTH_MASK;
        switch (depth)
        {
          case CV_8U:
          case CV_16U:
            is_signed = false;
            is_float  = false;
            break;
          case CV_8S:
          case CV_16S:
          case CV_32S:
            is_signed = true;
            is_float  = false;
            break;
          case CV_64F:
          case CV_32F:
            is_signed = true;
            is_float  = true;
            break;
        }

        CameraFrame image(frame.cols, frame.rows, frame.channels(),
                          static_cast<int>(frame.elemSize1()), is_signed, is_float, frame.data);
        OnFrameReceived(image);
      }
    }
  }
}

}  // namespace zebral