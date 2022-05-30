/// \file camera2cv.hpp
/// Not used in the library, but rather for converting from the camera images that the library
/// produces to OpenCV cv::Mat types for easy use.
#ifndef LIGHTBOX_CAMERA_CAMERA2CV_HPP_
#define LIGHTBOX_CAMERA_CAMERA2CV_HPP_

#include <opencv2/opencv.hpp>
#include "camera_frame.hpp"

namespace zebral
{
class Converter
{
 public:
  static int CVTypeFromImage(const CameraFrame& image)
  {
    int type_id = -1;

    switch (image.bytes_per_pixel())
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
          "Unsupported image type. bytesPerPixel:" + std::to_string(image.bytes_per_pixel()) +
          " signed:" + std::to_string(image.is_signed()) +
          " float:" + std::to_string(image.is_floating()));
    }

    return CV_MAKETYPE(type_id, image.channels());
  }

  static cv::Mat Camera2Cv(const CameraFrame& image)
  {
    cv::Mat img(image.height(), image.width(), CVTypeFromImage(image),
                const_cast<void*>(reinterpret_cast<const void*>(image.data())));
    return img;
  }

 private:
  Converter()  = delete;
  ~Converter() = delete;
};
}  // namespace zebral
#endif  // LIGHTBOX_CAMERA_CAMERA2CV_HPP_