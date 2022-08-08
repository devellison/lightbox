/// \file lightbox.hpp
/// Lightbox object - not implemented yet.
#ifndef LIGHTBOX_LIGHTBOX_HPP_
#define LIGHTBOX_LIGHTBOX_HPP_
#include "nano_inc.hpp"

#include "camera_http.hpp"
#include "errors.hpp"
#include "log.hpp"
#include "texture.hpp"
namespace zebral
{
/// Placeholder to control lightbox via USB serial.
/// Camera is under more active development :)
class Lightbox : public nanogui::Screen
{
 public:
  Lightbox();
  virtual ~Lightbox();
  void OpenCamera(const std::string& cameraPath, const std::string& user, const std::string& pwd);
  void CloseCamera();
  void OnUpdateFrame();
  void OnFrame(const CameraInfo& info, const CameraFrame& image);

  void drawContents() override;

  std::unique_ptr<Camera> camera_;
  nanogui::Window* imageWindow_;
  nanogui::ImageView* imageView_;
  mutable std::mutex frameMutex_;
  CameraFrame lastFrame_;
  bool newFrame_;
  Texture frameTexture_;
};

}  // namespace zebral
#endif  // LIGHTBOX_LIGHTBOX_HPP_