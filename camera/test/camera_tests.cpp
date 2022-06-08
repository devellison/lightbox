#if _WIN32
#define _CRT_DECLARE_NONSTDC_NAMES 1  // get unix-style read/write flags
#define _CRT_NONSTDC_NO_DEPRECATE     // c'mon, I just want to call open().
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <fcntl.h>
#include <atomic>
#include <cmath>
#include <filesystem>

#include "auto_close.hpp"
#include "camera_manager.hpp"
#include "camera_opencv.hpp"
#include "camera_winrt.hpp"
#include "errors.hpp"
#include "gtest/gtest.h"
#include "log.hpp"
#include "param.hpp"
#include "platform.hpp"

#if _WIN32
const int kDefaultMode = _S_IREAD | _S_IWRITE;
#else
const int kDefaultMode = S_IRUSR | S_IWUSR;
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

TEST(CameraTest, AutoClose)
{
  std::filesystem::path tempDir(::testing::TempDir());
  std::filesystem::path tempFile = tempDir / "AutoCloseTest";

  // Clean up if we've bombed a previous test
  if (std::filesystem::exists(tempFile))
  {
    ASSERT_TRUE(1 == std::filesystem::remove(tempFile));
  }

  {
    AutoClose outTest(::open(tempFile.string().c_str(), O_CREAT | O_RDWR | O_TRUNC, kDefaultMode));
    ASSERT_TRUE(outTest.valid());
    ASSERT_TRUE(4 == ::write(outTest.get(), "Test", 4));
  }

  ASSERT_TRUE(std::filesystem::exists(tempFile));
  std::cout << tempFile.string() << std::endl;
  ASSERT_TRUE(1 == std::filesystem::remove(tempFile));
  ASSERT_FALSE(std::filesystem::exists(tempFile));
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  // Create the platform for initialization of anything on this thread.
  Platform p;
  return RUN_ALL_TESTS();
}