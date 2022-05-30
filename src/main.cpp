#include <cstdlib>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "camera/camera.hpp"
#include "camera/camera_manager.hpp"
#include "camera/errors.hpp"
#include "lightbox.hpp"

using namespace zebral;

void printHelp(const std::vector<CameraInfo>& camList)
{
  std::cout << "lightbox v" << LIGHTBOX_VERSION << " by Michael Ellison." << std::endl;
  std::cout << "Usage: lightbox CAMERA_INDEX SERIAL_PORT" << std::endl;
  std::cout << "Cameras:" << std::endl;
  for (const auto& curCam : camList)
  {
    std::cout << "    " << curCam << std::endl;
  }
}

int main(int argc, char* argv[])
{
  SetUnhandled();

  CameraManager camMgr;
  auto camList = camMgr.Enumerate();

  // Check if we have enough arguments
  if (argc < 3)
  {
    printHelp(camList);
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
    std::cerr << "Could not create camera " << camIndex << std::endl;
    return to_int(Result::CAMERA_ERROR);
  }

  // Now open serial port

  // Start gui

  return 0;
}