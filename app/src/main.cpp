/// \file main.cpp
/// Main file for lightbox_app. Right now, just a test app
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include "camera.hpp"
#include "camera2cv.hpp"
#include "camera_manager.hpp"
#include "errors.hpp"
#include "lightbox.hpp"
#include "log.hpp"
#include "param.hpp"
#include "platform.hpp"

using namespace zebral;

void printHelp()
{
  std::cout << "lightbox v" << LIGHTBOX_VERSION << " by Michael Ellison " << std::endl;
  std::cout << "Usage: lightbox CAMERA_INDEX FORMAT [SERIAL_PORT]" << std::endl;
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
  if (argc < 2)
  {
    printHelp();
    return 0;
  }

  int camIndex = atoi(argv[1]);
  std::string format;

  if (argc >= 2)
  {
    format = argv[2];
  }

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
        f.width  = 640;
        f.height = 480;
        // camera->SetFormat(f, Camera::DecodeType::INTERNAL);
        camera->SetFormat(f);
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

  auto modeMaybe = camera->GetFormat();
  ZBA_ASSERT(modeMaybe, "Must get camera format");

  auto mode = *modeMaybe;
  cv::Rect roi(0, 0, 0, 0);

  bool first    = true;
  int param_idx = 0;

  do
  {
    if (first || pressed == '?')
    {
      std::cout << "Keys:" << std::endl;
      std::cout << "     r - roi" << std::endl;
      std::cout << "     i - info" << std::endl;
      std::cout << "     s - save image" << std::endl;
      std::cout << "     b - toggle buffering" << std::endl;
      std::cout << "     q - quit" << std::endl;
      std::cout << "     x - select next param " << std::endl;
      std::cout << "     y - toggle auto on a param" << std::endl;
      std::cout << "     - - reduce param" << std::endl;
      std::cout << "     = - increase param" << std::endl;
      std::cout << "     [ - min param" << std::endl;
      std::cout << "     ] - max param" << std::endl;
      std::cout << "     p - list params" << std::endl;
      first = false;
    }

    auto frame = camera->GetNewFrame();

    // Handle frame
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
      if (roi.empty())
      {
        cv::imshow("Camera", img);
      }
      else
      {
        cv::imshow("Camera", img(roi));
      }
    }
    else
    {
      ZBA_LOG("Got an empty frame in loop.");
    }

    switch (pressed)
    {
      case 'i':
        std::cout << camera->GetCameraInfo() << std::endl;
        break;
      case 'r':
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
        break;
      case 'p':
      {
        auto names = camera->GetParameterNames();
        for (size_t i = 0; i < names.size(); ++i)
        {
          std::cout << names[i];
          if (i == param_idx) std::cout << "*";
          std::cout << std::endl;
        }
      }
      break;
      case 'x':
      {
        auto names = camera->GetParameterNames();
        if (names.empty()) break;
        param_idx = (param_idx + 1) % (names.size());
        ZBA_LOG("Selected parameter {} ({})", names[param_idx], param_idx);
      }
      break;
      case 'y':
      {
        auto names = camera->GetParameterNames();
        if (names.empty()) break;
        auto param = camera->GetParameter(names[param_idx]);

        auto paramVal = dynamic_cast<ParamVal<double>*>(param.get());
        if (paramVal)
        {
          bool isAuto = paramVal->getAuto() ^ true;
          ZBA_LOG("Setting Auto {} to {}", names[param_idx], isAuto);
          paramVal->setAuto(isAuto);
          break;
        }
      }
      break;
      case '-':
      {
        auto names = camera->GetParameterNames();
        if (names.empty()) break;
        auto param = camera->GetParameter(names[param_idx]);

        auto ranged = dynamic_cast<ParamRanged<double, double>*>(param.get());
        if (ranged)
        {
          ranged->setScaled(ranged->getScaled() - ranged->getScaledStep());
          break;
        }

        auto menu = dynamic_cast<ParamMenu*>(param.get());
        if (menu)
        {
          auto index = menu->getIndex();
          if (index > 0)
          {
            index--;
          }
          menu->setIndex(index);
        }
      }
      break;
      case '=':
      {
        auto names = camera->GetParameterNames();
        if (names.empty()) break;
        auto param = camera->GetParameter(names[param_idx]);

        auto ranged = dynamic_cast<ParamRanged<double, double>*>(param.get());
        if (ranged)
        {
          ranged->setScaled(ranged->getScaled() + ranged->getScaledStep());
          break;
        }

        auto menu = dynamic_cast<ParamMenu*>(param.get());
        if (menu)
        {
          auto index = menu->getIndex();
          index      = (index + 1) % menu->getCount();
          menu->setIndex(index);
        }
      }
      break;
      case '[':
      {
        auto names = camera->GetParameterNames();
        if (names.empty()) break;
        auto param = camera->GetParameter(names[param_idx]);

        auto ranged = dynamic_cast<ParamRanged<double, double>*>(param.get());
        if (ranged)
        {
          ranged->setScaled(0);
          break;
        }

        auto menu = dynamic_cast<ParamMenu*>(param.get());
        if (menu)
        {
          menu->setIndex(0);
        }
      }
      break;
      case ']':
      {
        auto names = camera->GetParameterNames();
        if (names.empty()) break;
        auto param = camera->GetParameter(names[param_idx]);

        auto ranged = dynamic_cast<ParamRanged<double, double>*>(param.get());
        if (ranged)
        {
          ranged->setScaled(1);
          break;
        }

        auto menu = dynamic_cast<ParamMenu*>(param.get());
        if (menu)
        {
          menu->setIndex(menu->getCount() - 1);
        }
      }
      break;
      case 's':
        // save
        if (!img.empty())
        {
          if (!roi.empty())
          {
            saveImage(zba_now(), img(roi), imageDir);
          }
          else
          {
            saveImage(zba_now(), img, imageDir);
          }
        }
        break;
      case 'b':
        // buffer the image 10x and accum to denoise
        buffering ^= true;
        break;
    }
  } while ('q' != (pressed = cv::waitKey(10)));

  camera->Stop();
  return 0;
}