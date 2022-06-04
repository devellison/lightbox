#include <atomic>
#include "camera_manager.hpp"
#include "camera_opencv.hpp"
#include "camera_winrt.hpp"
#include "errors.hpp"
#include "gtest/gtest.h"
#include "log.hpp"
#include "platform.hpp"
#include "winrt_utils.hpp"

using namespace zebral;

TEST(CameraTests, ErrorsAndResults)
{
  ASSERT_TRUE(Failed(Result::ZBA_ERROR));
  ASSERT_TRUE(Failed(Result::ZBA_UNKNOWN_ERROR));
  ASSERT_TRUE(Success(Result::ZBA_SUCCESS));
  ASSERT_TRUE(Success(Result::ZBA_STATUS));

  ASSERT_FALSE(Success(Result::ZBA_ERROR));
  ASSERT_FALSE(Success(Result::ZBA_UNKNOWN_ERROR));
  ASSERT_FALSE(Failed(Result::ZBA_SUCCESS));
  ASSERT_FALSE(Failed(Result::ZBA_STATUS));

  EXPECT_THROW(ZBA_THROW("Testing", Result::ZBA_UNKNOWN_ERROR), Error);
}

// You need at least one source for this to pass...
// Probably remove it from the executed tests if run on a build server.
TEST(CameraTests, CameraSanity)
{
  CameraManager cmgr;
  auto camList = cmgr.Enumerate();
  ASSERT_TRUE(camList.size() > 0);

  int idx = 0;
  for (auto& curCam : camList)
  {
    ZBA_LOGSS(curCam);
    ASSERT_TRUE(curCam.index == idx);
    ASSERT_FALSE(curCam.name.empty());
    ASSERT_FALSE(curCam.path.empty());

    // Create each camera and dump its modes.
    auto camera = cmgr.Create(camList[idx]);
    auto info   = camera->GetCameraInfo();

    camera->SetFormat(info.formats[0]);
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

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  Platform p;
  return RUN_ALL_TESTS();
}