/// \file camera_frame.hpp
/// CameraFrame class for holding a camera image
#ifndef LIGHTBOX_CAMERA_CAMERA_FRAME_HPP_
#define LIGHTBOX_CAMERA_CAMERA_FRAME_HPP_

#include <type_traits>
#include <vector>

namespace zebral
{
/// Simple image class.
/// This is meant as a very simple wrapper for an image grabbed from a camera.
/// The intention is to not have the camera API depend on OpenCV, but to allow
/// easy integration with OpenCV or other libraries through a separate file.
///
/// It is NOT meant to be used for image processing - just holding the image from capture
/// until it's out of the library.
///
/// This will ONLY work for images where each pixel channel is 1 T currently,
/// If we decide to support packed formats this will need to get a lot more complex.
class CameraFrame
{
 public:
  CameraFrame()
      : width_(0),
        height_(0),
        channels_(0),
        bytes_per_channel_(0),
        is_signed_(false),
        is_floating_(false)
  {
  }

  CameraFrame(int width, int height, int channels, int bytesPerChannel, bool is_signed,
              bool is_floating_point, const uint8_t* data = nullptr)
      : width_(width),
        height_(height),
        channels_(channels),
        bytes_per_channel_(bytesPerChannel),
        is_signed_(is_signed),
        is_floating_(is_floating_point)
  {
    /// {TODO} won't work with stepped/padded data.
    size_t dataSize = width_ * height_ * channels_ * bytes_per_channel_;
    if (data)
    {
      data_.assign((data), (data + dataSize));
    }
    else
    {
      data_.resize(dataSize);
    }
  }

  bool empty() const
  {
    return data_.size() == 0;
  }

  int width() const
  {
    return width_;
  }

  int height() const
  {
    return height_;
  }

  int channels() const
  {
    return channels_;
  }

  int bytes_per_channel() const
  {
    return bytes_per_channel_;
  }

  int is_signed() const
  {
    return is_signed_;
  }

  bool is_floating() const
  {
    return is_floating_;
  }

  const size_t data_size() const
  {
    return data_.size();
  }

  const uint8_t* data() const
  {
    return data_.data();
  }

  uint8_t* data()
  {
    return data_.data();
  }

 protected:
  int width_;
  int height_;
  int channels_;
  int bytes_per_channel_;
  bool is_signed_;
  bool is_floating_;
  std::vector<uint8_t> data_;
};

}  // namespace zebral
#endif  // LIGHTBOX_CAMERA_CAMERA_FRAME_HPP_