/// \file common_tests.cpp
/// Unit tests and smoke tests for the common library
#if _WIN32
#define _CRT_DECLARE_NONSTDC_NAMES 1  // get unix-style read/write flags
#define _CRT_NONSTDC_NO_DEPRECATE     // c'mon, I just want to call open().
#define _CRT_SECURE_NO_WARNINGS
#endif  // _WIN32

#include <fcntl.h>
#include <atomic>
#include <cmath>
#include <filesystem>

#include "errors.hpp"
#include "find_files.hpp"
#include "gtest/gtest.h"
#include "log.hpp"
#include "platform.hpp"

using namespace zebral;

TEST(CommonTests, ErrorsAndResults)
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

TEST(CommonTests, FindFiles)
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
