#include <cstdlib>
#include <filesystem>
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
  std::cout << "Usage: lightbox CAMERA_INDEX SERIAL_PORT" << std::endl;
}

void saveImage(ZBA_TSTAMP time, const cv::Mat& img, const std::filesystem::path& path,
               const std::string identifier = "")
{
  std::filesystem::path savepath = path;
  std::string filename           = zba_format("image_{}", zba_local_time(time));
  if (!identifier.empty()) filename += "_" + identifier;
  filename += ".png";
  savepath /= filename;

  imwrite(savepath.string(), img);
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
  std::string serial = argv[2];
  std::shared_ptr<Camera> camera;

  for (auto& curCam : camList)
  {
    // Create it
    if (curCam.index == camIndex)
    {
      camera    = camMgr.Create(curCam);
      auto info = camera->GetCameraInfo();
      if (info.formats.size())
      {
        // Largest 30fps format.
        FormatInfo f;
        f.fps = 30;
        // f.format = "NV12";
        // camera->SetFormat(f, false);
        camera->SetFormat(f);
      }
    }
  }

  // Bail if we don't have a camera
  if (!camera)
  {
    std::cerr << "Could not create camera: " << camIndex << std::endl;
    return to_int(Result::ZBA_CAMERA_ERROR);
  }

  // Create image directory
  constexpr static auto kImageDir = "images";
  std::filesystem::path imageDir(kImageDir);
  if (!std::filesystem::exists(imageDir))
  {
    std::filesystem::create_directory(imageDir);
    if (!std::filesystem::exists(imageDir))
    {
      ZBA_ERR("WARNING: images folder does not exist!");
      imageDir = std::filesystem::temp_directory_path();
      ZBA_ERR("Saving images to {} instead.", imageDir.string());
    }
  }

  // Now open serial port

  // Start gui
  camera->Start();
  bool buffering = false;
  int pressed    = 0;
  std::vector<cv::Mat> bufferMats;
  cv::Mat accum;
  const int kMaxBufferSize = 10;

  auto formatMaybe = camera->GetFormat();
  ZBA_ASSERT(formatMaybe, "Must get camera format");
  auto format = *formatMaybe;

  cv::Rect roi(0, 0, format.width, format.height);

  do
  {
    auto frame = camera->GetNewFrame();
    cv::Mat img;
    if (frame)
    {
      img = Converter::Camera2Cv(*frame);
      if (buffering)
      {
        if (accum.empty())
        {
          accum = cv::Mat::zeros(img.rows, img.cols, CV_64FC3);
        }

        cv::Mat adder;
        img.convertTo(adder, CV_64FC3);
        bufferMats.push_back(adder);
        accum += adder;

        while (bufferMats.size() > kMaxBufferSize)
        {
          accum -= (*bufferMats.begin());
          bufferMats.erase(bufferMats.begin());
        }

        img = accum / static_cast<double>(bufferMats.size());
        img.convertTo(img, CV_8UC3);
      }
      cv::imshow("Camera", img(roi));
    }
    else
    {
      ZBA_LOG("Got an empty frame in loop.");
    }
    switch (pressed)
    {
      case 'r':
      {
        // set ROI
        if ((roi.width != img.cols) || (roi.height != img.rows))
        {
          roi = cv::Rect(0, 0, img.cols, img.rows);
        }
        else
        {
          cv::destroyWindow("Camera");
          roi = cv::selectROI(img, true);
          cv::destroyWindow("ROI selector");
        }
      }
      break;
      case 'i':
      {
        // camera info
        std::cout << camera->GetCameraInfo() << std::endl;
      }
      break;
      case 's':
      {
        // save
        if (!img.empty())
        {
          saveImage(zba_now(), img(roi), imageDir);
        }
      }
      break;
      case 'x':
      {
        // Split channels
        std::vector<cv::Mat> array;
        cv::split(img(roi), array);
        auto curTime = zba_now();
        saveImage(curTime, array[0], imageDir, "blue");
        saveImage(curTime, array[1], imageDir, "green");
        saveImage(curTime, array[2], imageDir, "red");
        cv::equalizeHist(array[0], array[0]);
        cv::equalizeHist(array[1], array[1]);
        cv::equalizeHist(array[2], array[2]);
        saveImage(curTime, array[0], imageDir, "blue_eq");
        saveImage(curTime, array[1], imageDir, "green_eq");
        saveImage(curTime, array[2], imageDir, "red_eq");
      }
      case 'b':
      {
        // buffer the image 10x and accum to denoise
        buffering ^= true;
      }
      break;
    }
  } while ('q' != (pressed = cv::waitKey(10)));

  camera->Stop();
  return 0;
}