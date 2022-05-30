#include "errors.hpp"
#include "gtest/gtest.h"
#include "log.hpp"

using namespace zebral;


int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}