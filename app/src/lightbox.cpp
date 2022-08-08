/// \file lightbox.cpp
/// Implementation of Lightbox app.  Not implemented yet.
#include "lightbox.hpp"

#include <functional>
#include <memory>

#ifndef LIGHTBOX_VERSION
#define LIGHTBOX_VERSION "0.0.0"
#endif

namespace zebral
{
static const char* kLightboxTitle = "Lightbox";

Lightbox::Lightbox()
    : nanogui::Screen(Eigen::Vector2i(1024, 768), kLightboxTitle, true),
      imageWindow_(nullptr),
      imageView_(nullptr)
{
  Screen::setVisible(true);
  Screen::performLayout();
}

Lightbox::~Lightbox()
{
  Lightbox::CloseCamera();
}

void Lightbox::OpenCamera(const std::string& uri, const std::string& user, const std::string& pwd)
{
  camera_.reset(new CameraHttp("Camera", uri, user, pwd));
  auto onFrame = std::bind(&Lightbox::OnFrame, this, std::placeholders::_1, std::placeholders::_2);
  camera_->Start(onFrame);
}

void Lightbox::CloseCamera()
{
  if (camera_)
  {
    camera_.reset();
  }
}

void Lightbox::drawContents()
{
  OnUpdateFrame();
  Screen::drawContents();
}

void Lightbox::OnUpdateFrame()
{
  if (!newFrame_) return;
  int w = 320;
  int h = 200;
  {
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (lastFrame_.width() <= 0) return;
    frameTexture_.Load(lastFrame_);
    newFrame_ = false;
    w         = lastFrame_.width();
    h         = lastFrame_.height();
  }

  if (!imageWindow_)
  {
    imageWindow_ = new nanogui::Window(this, camera_->GetCameraInfo().name);
    imageWindow_->setLayout(new nanogui::GroupLayout());
    imageWindow_->setSize(Eigen::Vector2i(w, h));
    imageView_ = new nanogui::ImageView(imageWindow_, frameTexture_.Id());
    imageView_->setVisible(true);
    performLayout();
  }
  else
  {
    //  imageView_->bindImage(frameTexture_.Id());
  }
}

void Lightbox::OnFrame(const CameraInfo& info, const CameraFrame& image)
{
  (void)info;
  {
    std::lock_guard<std::mutex> lock(frameMutex_);
    lastFrame_ = image;
    newFrame_  = true;
  }
}

}  // namespace zebral
