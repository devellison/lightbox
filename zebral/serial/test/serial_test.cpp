#include "errors.hpp"
#include "gtest/gtest.h"
#include "log.hpp"
#include "serial_channel.hpp"

using namespace zebral;

TEST(SerialTest, Enumeration)
{
  auto devices = SerialChannel::Enumerate();
  for (const auto& curDevice : devices)
  {
    std::cout << curDevice << std::endl;
  }
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}