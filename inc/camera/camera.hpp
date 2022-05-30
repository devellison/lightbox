#ifndef LIGHTBOX_CAMERA_HPP_
#define LIGHTBOX_CAMERA_HPP_

#include <functional>
#include <opencv2/opencv.hpp>
#include <thread>

namespace zebral
{
/// Camera base class
class Camera
{
 public:
  typedef std::function<void(const cv::Mat& image)> ImageCallback;

  Camera();
  virtual ~Camera() = default;

  void Start(ImageCallback cb = nullptr);
  void Stop();

  cv::Mat GetNextFrame();

 protected:
  ImageCallback callback_;
  std::thread camera_thread_;
  std::mutex frame_mutex_;
};

}  // namespace zebral
#endif  // #define LIGHTBOX_CAMERA_HPP_