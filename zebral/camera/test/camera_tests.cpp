/// \file camera_tests.cpp
/// Unit tests and smoke tests for the camera library
#if _WIN32
#define _CRT_DECLARE_NONSTDC_NAMES 1  // get unix-style read/write flags
#define _CRT_NONSTDC_NO_DEPRECATE     // c'mon, I just want to call open().
#define _CRT_SECURE_NO_WARNINGS
#endif  // _WIN32

#include <fcntl.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <filesystem>

#include "camera_manager.hpp"
#include "camera_platform.hpp"
#include "convert.hpp"
#include "errors.hpp"
#include "find_files.hpp"
#include "gtest/gtest.h"
#include "log.hpp"
#include "param.hpp"
#include "platform.hpp"

/// So rude. Something's defining max on me and causing errors with std::max
#ifdef max
#undef max
#endif

using namespace zebral;

TEST(CameraTests, ErrorsAndResults)
{
  // Sanity check on errors
  ASSERT_TRUE(Failed(Result::ZBA_ERROR));
  ASSERT_TRUE(Failed(Result::ZBA_UNKNOWN_ERROR));
  ASSERT_TRUE(Success(Result::ZBA_SUCCESS));
  ASSERT_TRUE(Success(Result::ZBA_STATUS));

  ASSERT_FALSE(Success(Result::ZBA_ERROR));
  ASSERT_FALSE(Success(Result::ZBA_UNKNOWN_ERROR));
  ASSERT_FALSE(Failed(Result::ZBA_SUCCESS));
  ASSERT_FALSE(Failed(Result::ZBA_STATUS));

  EXPECT_THROW(ZBA_THROW("Testing", Result::ZBA_UNKNOWN_ERROR), Error);
  // Sanity check on throws
  try
  {
    ZBA_THROW("Camera failed exception test", Result::ZBA_CAMERA_OPEN_FAILED);
  }
  catch (const Error& error)
  {
    ASSERT_TRUE(strlen(error.what()) != 0);
    ASSERT_TRUE(error.why() == Result::ZBA_CAMERA_OPEN_FAILED);
    ASSERT_TRUE(error.where().length() != 0);
  }
}

TEST(CameraTests, ConvertSpeeds)
{
  CameraManager cmgr;
  auto camList = cmgr.Enumerate();
  if (camList.size() == 0)
  {
    ZBA_ERR("NO CAMERAS FOUND, SKIPPING TESTS");
    return;
  }
  auto camera = cmgr.Create(camList[0]);
  FormatInfo f;
  f.fps    = 30;
  f.width  = 640;
  f.height = 480;
#if _WIN32
  f.format = "YUY2";
#else
  f.format = "YUYV";
#endif
  camera->SetFormat(f, Camera::DecodeType::NONE);
  camera->Start();
  auto maybeFrame = camera->GetNewFrame();
  if (maybeFrame)
  {
    auto frame = *maybeFrame;
    CameraFrame f1, f2;
    YUV2RGB = YUVToRGB;

    auto start1 = zba_now();
    for (int i = 0; i < 100; i++)
    {
      f1 = YUY2ToBGRFrame(frame.data(), frame.width(), frame.height(),
                          frame.width() * frame.channels() * frame.bytes_per_channel());
    }
    auto time1 = zba_elapsed_sec(start1);

    YUV2RGB     = YUVToRGBFixed;
    auto start2 = zba_now();
    for (int i = 0; i < 100; i++)
      f2 = YUY2ToBGRFrame(frame.data(), frame.width(), frame.height(),
                          frame.width() * frame.channels() * frame.bytes_per_channel());
    auto time2 = zba_elapsed_sec(start2);
    ZBA_LOG("Float: {}  Fixed: {}", time1, time2);
    ZBA_ASSERT(time1 > time2, "Fixed should be much faster");

    if (0 != memcmp(f1.data(), f2.data(), f1.data_size()))
    {
      ZBA_ERR("Pixels different!");
      size_t diff  = 0;
      int max_diff = 0;
      for (size_t i = 0; i < f1.data_size(); i++)
      {
        uint8_t b1 = f1.data()[i];
        uint8_t b2 = f2.data()[i];
        if (b1 != b2)
        {
          max_diff = std::max(std::abs(b1 - b2), max_diff);

          // ZBA_ERR("{}: {} vs {}", i, b1, b2);
          ++diff;
        }
      }
      ZBA_ERR("{} pixels different out of {}", diff, f1.data_size());
      ZBA_ASSERT(max_diff <= 1, "Error must be rounding, not a real error!");
    }
  }

  camera->Stop();
}

// You need at least one source for this to test stuff.
// If not, it passes unit tests but skips a lot of them.
TEST(CameraTests, CameraSanity)
{
  CameraManager cmgr;
  auto camList = cmgr.Enumerate();
  if (camList.size() == 0)
  {
    ZBA_ERR("NO CAMERAS FOUND, SKIPPING TESTS");
    return;
  }

  int idx = 0;
  for (auto& curCam : camList)
  {
    ZBA_LOGSS(curCam);
    // This is no longer a valid check, as V4L2 we're using
    // the idx for the device number. E.g. /dev/video4 is 4.
    // /dev/video* become non-contiguous with disconnects
    // ASSERT_TRUE(curCam.index == idx);

    // All platforms should have a name for the device.
    ASSERT_FALSE(curCam.name.empty());

    // Windows path is empty currently.
#if __linux__
    // Should be /dev/video0 or similar
    ASSERT_FALSE(curCam.path.empty());
#endif

    // Check bus path - win/linux both get a (completely different) value here.
    ASSERT_FALSE(curCam.bus.empty());

    // Create each camera and dump its modes.
    ZBA_TIMER(camera_timer, std::string("Camera {}"), curCam.name);
    auto camera = cmgr.Create(curCam);
    auto info   = camera->GetCameraInfo();

    camera_timer.log("Created");

    ZBA_LOGSS(info);

    if (info.formats.size())
    {
      /// Pick the first format with 30 fps.
      FormatInfo f;
      f.fps = 30;
      camera->SetFormat(f);
    }
    else
    {
      // No formats, skip it.
      ZBA_LOG("No formats on camera {}, skipping...", idx);
      idx++;
      continue;
    }
    camera_timer.log("Format set");

    ZBA_LOG("Starting {}...", info.name.c_str());

    int count = 0;

    TimeStamp lastTimestamp = TimeStampNow();
    // Callback method - start it and let it pump the frames
    auto frameCallback = [&](const CameraInfo&, const CameraFrame& image, TimeStamp timestamp)
    {
      ASSERT_TRUE(image.empty() == false);
      ASSERT_TRUE(timestamp > lastTimestamp);
      lastTimestamp = timestamp;
      count++;
    };

    const auto kWaitForFramesTime = 2;

    camera->Start(frameCallback);
    std::this_thread::sleep_for(std::chrono::seconds(kWaitForFramesTime));
    camera->Stop();
    ASSERT_TRUE(count > 1);
    ZBA_LOG("{} frames in a second", static_cast<int>(count));

    count = 0;
    // Direct call method - start it, then ask for new frames.
    camera->Start();
    auto start = zba_now();
    while (zba_elapsed_sec(start) < kWaitForFramesTime)
    {
      auto frame = camera->GetNewFrame(1000);
      ASSERT_TRUE(frame);
      count++;
    }
    ASSERT_TRUE(count > 1);
    ZBA_LOG("Stopping {}", info.name.c_str());
    camera->Stop();

    //-----
    ++idx;
  }
}

double IntToUnit(int value, int minVal, int maxVal)
{
  if (maxVal == minVal)
  {
    ZBA_THROW("Invalid range", Result::ZBA_INVALID_RANGE);
  }
  return static_cast<double>((value - minVal)) / static_cast<double>(maxVal - minVal);
}

int UnitToInt(double scaled, int minVal, int maxVal)
{
  return static_cast<int>(std::round((scaled * (maxVal - minVal) + minVal)));
}

class ChangeWatch
{
 public:
  ChangeWatch() : deviceChanges(0), guiChanges(0) {}

  void OnVolumeChangedGUI(Param* param, bool raw_set)
  {
    auto p = dynamic_cast<ParamRanged<int, double>*>(param);
    if (p && !raw_set)
    {
      ZBA_LOG("Volume changed - {} ({})", p->get(), p->getScaled());
      guiChanges++;
    }
  }

  void OnVolumeChangedDevice(Param* param, bool raw_set)
  {
    auto p = dynamic_cast<ParamRanged<int, double>*>(param);
    if (p && (raw_set))
    {
      ZBA_LOG("Volume changed (device) - {} ({})", p->get(), p->getScaled());
      deviceChanges++;
    }
  }

  int deviceChanges;
  int guiChanges;
};

TEST(CameraTests, Params)
{
  ParamSubscribers callbacks;
  ChangeWatch watch;
  ParamCb guiCb    = std::bind(&ChangeWatch::OnVolumeChangedGUI, &watch, std::placeholders::_1,
                               std::placeholders::_2);
  ParamCb deviceCb = std::bind(&ChangeWatch::OnVolumeChangedDevice, &watch, std::placeholders::_1,
                               std::placeholders::_2);
  callbacks.insert({"Gui", guiCb});
  callbacks.insert({"Device", deviceCb});
  ParamRanged<int, double> volume("Volume", callbacks, 25, 50, 0, 100, IntToUnit, UnitToInt);

  // Make sure it's registering the changes with callbacks and returning the right
  // values for clamping
  ASSERT_FALSE(volume.set(10));
  ASSERT_TRUE(watch.deviceChanges == 1);
  ASSERT_TRUE(watch.guiChanges == 0);
  ASSERT_TRUE(volume.get() == 10);
  ASSERT_FALSE(volume.set(100));
  ASSERT_TRUE(watch.deviceChanges == 2);
  ASSERT_TRUE(volume.get() == 100);

  ASSERT_TRUE(volume.set(150));
  ASSERT_TRUE(volume.get() == 100);
  ASSERT_FALSE(volume.setScaled(0));

  // Not 3 because it didn't change when 150 got clamped,
  // because it was already at 100.
  ASSERT_TRUE(watch.deviceChanges == 2);

  ASSERT_TRUE(watch.guiChanges == 1);
  ASSERT_TRUE(volume.get() == 0);
  ASSERT_FALSE(volume.setScaled(0.5));
  ASSERT_TRUE(volume.get() == 50);
  ASSERT_FALSE(volume.setScaled(0.9));
  ASSERT_TRUE(volume.get() == 90);
  ASSERT_FALSE(volume.setScaled(1.0));
  ASSERT_TRUE(volume.get() == 100);

  ASSERT_TRUE(volume.setScaled(1.5));
  ASSERT_TRUE(volume.get() == 100);
  ASSERT_TRUE(watch.guiChanges == 5);
  ASSERT_TRUE(watch.deviceChanges == 2);
}

TEST(CameraTests, FindFiles)
{
  ZBA_LOG("Current Dir: {}", std::filesystem::current_path().string().c_str());
  /**
   * {TODO} Need to set up some tests for find file for xplat.
   */
  /*
  auto video_devs = FindFiles("/dev/", "video([0-9]+)");
  for (auto curMatch : video_devs)
  {
   std::string path = curMatch.dir_entry.path().string();
    ZBA_LOG("Checking {}", path.c_str());
    for (size_t i = 0; i < curMatch.matches.size(); ++i)
    {
      ZBA_LOG("Match {}: {}",i,curMatch.matches[i].c_str());
    }
    int idx = std::stoi(curMatch.matches[1]);
    ZBA_LOG("idx: {}",idx);
  }
  */
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  // Create the platform for initialization of anything on this thread.
  Platform p;
  return RUN_ALL_TESTS();
}
