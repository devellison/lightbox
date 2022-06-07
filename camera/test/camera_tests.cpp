#include <atomic>
#include <cmath>
#include "camera_manager.hpp"
#include "camera_opencv.hpp"
#include "camera_winrt.hpp"
#include "errors.hpp"
#include "gtest/gtest.h"
#include "log.hpp"
#include "param.hpp"
#include "platform.hpp"

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
    ASSERT_TRUE(curCam.index == idx);
/// {TODO} Add this back in when we're on V4L2
#if _WIN32
    ASSERT_FALSE(curCam.name.empty());
    ASSERT_FALSE(curCam.path.empty());
#endif

    // Create each camera and dump its modes.
    auto camera = cmgr.Create(camList[idx]);
    auto info   = camera->GetCameraInfo();

    if (info.formats.size())
    {
      camera->SetFormat(info.formats[0]);
    }
    ZBA_LOG("Starting %s...", info.name.c_str());

    int count = 0;

    TimeStamp lastTimestamp = TimeStampNow();
    // Callback method - start it and let it pump the frames
    auto frameCallback = [&](const CameraInfo&, const CameraFrame& image, TimeStamp timestamp) {
      ASSERT_TRUE(image.empty() == false);
      ASSERT_TRUE(timestamp > lastTimestamp);
      lastTimestamp = timestamp;
      count++;
    };

    camera->Start(frameCallback);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    camera->Stop();
    ASSERT_TRUE(count > 1);
    ZBA_LOG("%d frames in a second", static_cast<int>(count));

    count = 0;
    // Direct call method - start it, then ask for new frames.
    camera->Start();
    auto start = zba_now();
    while (zba_elapsed_sec(start) < 1)
    {
      auto frame = camera->GetNewFrame(1000);
      ASSERT_TRUE(frame);
      count++;
    }
    ASSERT_TRUE(count > 1);
    ZBA_LOG("Stopping %s", info.name.c_str());
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
      ZBA_LOG("Volume changed - %d (%f)", p->get(), p->getScaled());
      guiChanges++;
    }
  }

  void OnVolumeChangedDevice(Param* param, bool raw_set)
  {
    auto p = dynamic_cast<ParamRanged<int, double>*>(param);
    if (p && (raw_set))
    {
      ZBA_LOG("Volume changed (device) - %d (%f)", p->get(), p->getScaled());
      deviceChanges++;
    }
  }

  int deviceChanges;
  int guiChanges;
};

TEST(CameraTests, Params)
{
  SubscriberList callbacks;
  ChangeWatch watch;
  ParamCb guiCb    = std::bind(&ChangeWatch::OnVolumeChangedGUI, &watch, std::placeholders::_1,
                            std::placeholders::_2);
  ParamCb deviceCb = std::bind(&ChangeWatch::OnVolumeChangedDevice, &watch, std::placeholders::_1,
                               std::placeholders::_2);
  ParamChangedCb guicallback{"Gui", guiCb};
  ParamChangedCb devicecallback{"Device", deviceCb};
  callbacks.insert(guicallback);
  callbacks.insert(devicecallback);
  ParamRanged<int, double> volume("Volume", callbacks, 25, 50, 0, 100, IntToUnit, UnitToInt);
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

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  // Create the platform for initialization of anything on this thread.
  Platform p;
  return RUN_ALL_TESTS();
}