/// \file main.cpp
/// Main file for lightbox_app. Right now, just a test app
#include <cstdlib>
#include <iostream>
#include "camera.hpp"
#include "camera2cv.hpp"
#include "camera_manager.hpp"
#include "errors.hpp"
#include "lightbox.hpp"
#include "log.hpp"
#include "platform.hpp"

// This should replace all of the above, mostly.
#include "zebral_camera.hpp"

using namespace zebral;

void printHelp()
{
  std::cout << "lightbox v" << LIGHTBOX_VERSION << " by Michael Ellison " << std::endl;
  std::cout << "Usage: lightbox CAMERA_INDEX FORMAT [SERIAL_PORT]" << std::endl;
}

int main(int argc, char* argv[])
{
  Platform platform;

  std::cout << "Scanning cameras..." << std::endl;
  CameraManager camMgr;
  auto camList = camMgr.Enumerate();

  // Check if we have enough arguments
  if (argc < 3)
  {
    printHelp();
    return 0;
  }

  int camIndex       = atoi(argv[1]);
  std::string format = argv[2];
  std::shared_ptr<Camera> camera;

  int idx = 0;
  for (auto& curCam : camList)
  {
    // Create it
    if (idx == camIndex)
    {
      std::cout << "Selected camera:" << curCam << std::endl;
      camera    = camMgr.Create(curCam);
      auto info = camera->GetCameraInfo();
      if (info.formats.size())
      {
        // Largest 30fps format.
        FormatInfo f;
        f.fps    = 30;
        f.format = format;
        camera->SetFormat(f, false);
        // camera->SetFormat(f);
      }
    }
    idx++;
  }

  // Bail if we don't have a camera
  if (!camera)
  {
    std::cerr << "Could not create camera: " << camIndex << std::endl;
    return to_int(Result::ZBA_CAMERA_ERROR);
  }

  // Now open serial port

  // Start gui
  camera->Start();

  int pressed = 0;

  do
  {
    if (pressed == 'i')
    {
      std::cout << camera->GetCameraInfo() << std::endl;
    }
    auto frame = camera->GetNewFrame();
    if (frame)
    {
      cv::Mat img = Converter::Camera2Cv(*frame);
      if (img.type() == CV_16UC1)
      {
        // Just make it a bit more visible for now.
        // This is generally a depth camera.
        img.convertTo(img, CV_8UC1, 1.0 / 257.0);
        cv::equalizeHist(img, img);
      }
      cv::imshow("Camera", img);
    }
    else
    {
      ZBA_LOG("Got an empty frame in loop.");
    }
  } while ('q' != (pressed = cv::waitKey(10)));

  camera->Stop();
  return 0;
}