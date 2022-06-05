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
  /// Default ctor, just zeros everything.
  /// empty() will return true.
  CameraFrame()
      : width_(0),
        height_(0),
        channels_(0),
        bytes_per_channel_(0),
        is_signed_(false),
        is_floating_(false)
  {
  }

  /// Normal ctor - if data is provided, copies it into the vector.
  /// Otherwise, reserves space for it.
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

  /// Returns true if the data buffer has no data.
  bool empty() const
  {
    return data_.size() == 0;
  }

  /// Pixel width of the image
  int width() const
  {
    return width_;
  }

  /// Pixel height of the image
  int height() const
  {
    return height_;
  }

  /// Number of channels (e.g. RGBA = 4)
  int channels() const
  {
    return channels_;
  }

  /// Bytes per pixel per channel, so in RGBA with 8-bits per channel per pixel, is 1
  int bytes_per_channel() const
  {
    return bytes_per_channel_;
  }

  /// Is the data a signed type?
  int is_signed() const
  {
    return is_signed_;
  }

  /// Is the data a floating type?
  bool is_floating() const
  {
    return is_floating_;
  }

  /// Size of image data in bytes
  size_t data_size() const
  {
    return data_.size();
  }

  /// Pointer to start of image data
  const uint8_t* data() const
  {
    return data_.data();
  }

  /// Pointer to start of image data
  uint8_t* data()
  {
    return data_.data();
  }

 protected:
  int width_;                  ///< width of image in pixels
  int height_;                 ///< height of image in pixels
  int channels_;               ///< number of channels (expect interleaved channels in data)
  int bytes_per_channel_;      ///< bytes per pixel per channel
  bool is_signed_;             ///< is data a signed type?
  bool is_floating_;           ///< is data a floating type?
  std::vector<uint8_t> data_;  ///< Vector to store data
};

}  // namespace zebral
#endif  // LIGHTBOX_CAMERA_CAMERA_FRAME_HPP_