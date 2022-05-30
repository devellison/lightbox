#include <cstdlib>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "camera.hpp"
#include "camera2cv.hpp"
#include "camera_manager.hpp"
#include "errors.hpp"
#include "lightbox.hpp"
#include "log.hpp"
#include "platform.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace zebral;

void printHelp()
{
  ZEBRAL_LOG("lightbox v%s by Michael Ellison", LIGHTBOX_VERSION);
  ZEBRAL_LOG("Usage: lightbox CAMERA_INDEX SERIAL_PORT");
}

int main(int argc, char* argv[])
{
  Platform platform;

  ZEBRAL_LOG("Scanning cameras...");
  CameraManager camMgr;
  auto camList = camMgr.Enumerate();
  for (const auto& curCam : camList)
  {
    ZEBRAL_LOGSS(curCam);
  }

  // Check if we have enough arguments
  if (argc < 3)
  {
    printHelp();
    return 0;
  }

  int camIndex       = atoi(argv[1]);
  std::string serial = argv[2];
  std::shared_ptr<Camera> camera;

  for (auto& curCam : camList)
  {
    // Create it
    if (curCam.index == camIndex)
    {
      camera = camMgr.Create(curCam);
    }
  }

  // Bail if we don't have a camera
  if (!camera)
  {
    ZEBRAL_LOG("Could not create camera %d", camIndex);
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
      ZEBRAL_LOGSS(camera->GetCameraInfo());
    }
    auto frame = camera->GetNewFrame();
    if (frame)
    {
      cv::Mat img = Converter::Camera2Cv(*frame);
      cv::imshow("Camera", img);
    }
    else
    {
      ZEBRAL_LOG("Got an empty frame in loop.");
    }
  } while ('q' != (pressed = cv::waitKey(10)));

  camera->Stop();
  return 0;
}