#include "gtest/gtest.h"
#include "platform.hpp"

using namespace zebral;

int main(int argc, char** argv)
{
  Platform p;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}