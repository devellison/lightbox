/// \file zebral_test.cpp
/// Unit test for shared library - just make sure it's connecting with the shared lib.
/// Static libraries have the bulk of tests.

#include "camera_manager.hpp"
#include "camera_platform.hpp"
#include "convert.hpp"
#include "errors.hpp"
#include "find_files.hpp"
#include "gtest/gtest.h"
#include "log.hpp"
#include "param.hpp"
#include "platform.hpp"

using namespace zebral;

TEST(ZebralTest, SanityCheck)
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
  ASSERT_TRUE(maybeFrame);
  camera->Stop();
  ASSERT_FALSE(camList[0].name.empty());
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  // Create the platform for initialization of anything on this thread.
  Platform p;
  return RUN_ALL_TESTS();
}
