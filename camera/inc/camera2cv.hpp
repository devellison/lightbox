/// \file camera2cv.hpp
/// Not used in the library, but rather for converting from the camera images that the library
/// produces to OpenCV cv::Mat types for easy use.
#ifndef LIGHTBOX_CAMERA_CAMERA2CV_HPP_
#define LIGHTBOX_CAMERA_CAMERA2CV_HPP_

#include <opencv2/opencv.hpp>
#include "camera_frame.hpp"
#include "errors.hpp"

namespace zebral
{
class Converter
{
 public:
  static cv::Mat Camera2Cv(const CameraFrame& image)
  {
    return cv::Mat(image.height(), image.width(), CVTypeFromImage(image),
                   const_cast<void*>(reinterpret_cast<const void*>(image.data())));
  }

  CameraFrame CvToCameraFrame(const cv::Mat& frame)
  {
    if (frame.empty()) return CameraFrame();
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
      default:
        throw Error("Unsupported cv::Mat type:" + std::to_string(frame.type()),
                    Result::ZBA_UNSUPPORTED_FMT);
    }

    return CameraFrame(frame.cols, frame.rows, frame.channels(),
                       static_cast<int>(frame.elemSize1()), is_signed, is_float, frame.data);
  }
  static int CVTypeFromImage(const CameraFrame& image)
  {
    int type_id = -1;

    switch (image.bytes_per_channel())
    {
      case 1:
        type_id = image.is_signed() ? CV_8S : CV_8U;
        break;
      case 2:
        type_id = image.is_signed() ? CV_16S : CV_16U;
        break;
      case 4:
        if (image.is_floating())
        {
          type_id = CV_32F;
        }
        else if (image.is_signed())
        {
          type_id = CV_32S;
        }
        break;
      case 8:
        if (image.is_floating())
        {
          type_id = CV_64F;
        }
        break;
      default:
        break;
    }
    if (type_id == -1)
    {
      throw std::runtime_error(
          "Unsupported image type. bytes_per_channel:" + std::to_string(image.bytes_per_channel()) +
          " signed:" + std::to_string(image.is_signed()) +
          " float:" + std::to_string(image.is_floating()));
    }

    return CV_MAKETYPE(type_id, image.channels());
  }

 private:
  Converter()  = delete;
  ~Converter() = delete;
};
}  // namespace zebral
#endif  // LIGHTBOX_CAMERA_CAMERA2CV_HPP_