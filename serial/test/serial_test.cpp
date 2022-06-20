#include "errors.hpp"
#include "gtest/gtest.h"
#include "log.hpp"
#include "serial_line_channel.hpp"

using namespace zebral;
TEST(SerialTest, Enumeration)
{
#if _WIN32
  [[maybe_unused]] auto devices = SerialLineChannel::Enumerate();
#endif
}
int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}