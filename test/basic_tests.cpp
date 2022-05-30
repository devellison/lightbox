#include <errors.hpp>
#include "gtest/gtest.h"

using namespace zebral;

TEST(BasicTests, ErrorsAndResults)
{
  ASSERT_TRUE(Failed(Result::ERROR));
  ASSERT_TRUE(Failed(Result::UNKNOWN_ERROR));
  ASSERT_TRUE(Success(Result::SUCCESS));
  ASSERT_TRUE(Success(Result::STATUS));

  ASSERT_FALSE(Success(Result::ERROR));
  ASSERT_FALSE(Success(Result::UNKNOWN_ERROR));
  ASSERT_FALSE(Failed(Result::SUCCESS));
  ASSERT_FALSE(Failed(Result::STATUS));

  EXPECT_THROW(ZEBRAL_THROW("Testing", Result::UNKNOWN_ERROR), Error);
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}