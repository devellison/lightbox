#include "camera_manager.hpp"
#include "camera_opencv.hpp"
#include "errors.hpp"
#include "gtest/gtest.h"
#include "log.hpp"

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

  EXPECT_THROW(ZEBRAL_THROW("Testing", Result::ZBA_UNKNOWN_ERROR), Error);
}

TEST(CameraTests, Time)
{
  // Decent visual test, not so useful for running automated each time.
  // At some point, capture stdout and verify.
  /*
  auto start_time = zebral_now();
  int i           = 0;
  ZEBRAL_LOG("Start counting...");
  while (zebral_elapsed_sec(start_time) < 2.0)
  {
    ZEBRAL_LOG("Counting %d", i);
    ++i;
  }
  ZEBRAL_LOG("End counting...");
  */
}

// You need at least one source for this to pass...
// Probably remove it from the executed tests if run on a build server.
TEST(CameraTests, CameraEnum)
{
  CameraManager cmgr;
  auto camList = cmgr.Enumerate();
  ASSERT_TRUE(camList.size() > 0);

  int idx = 0;
  for (auto& curCam : camList)
  {
    ZEBRAL_LOGSS(curCam);
    ASSERT_TRUE(curCam.index == idx);
    ASSERT_FALSE(curCam.name.empty());
    ASSERT_FALSE(curCam.path.empty());
    ++idx;
  }
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}