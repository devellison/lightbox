#if __linux__
#include <fstream>
#include "find_files.hpp"
#include "platform.hpp"
#include "serial_line_channel.hpp"

namespace zebral
{
SerialLineChannel::SerialLineChannel(const SerialInfo& info) : info_(info) {}
SerialLineChannel::~SerialLineChannel() {}

std::vector<SerialInfo> SerialLineChannel::Enumerate()
{
  /// {TODO} right now we're just enumerating USB CDC_ACM devices (modern USB serial devices)
  ///        should probably extend this to other types of serial, but these do what I need.
  std::vector<SerialInfo> serialList;

  auto serial_devs = FindFiles("/dev/", "ttyACM([0-9]+)");
  for (auto curMatch : serial_devs)
  {
    std::string path      = curMatch.dir_entry.path().string();
    std::string path_file = curMatch.dir_entry.path().filename().string();

    /// {TODO} fill these out
    std::string name;
    std::string bus;
    uint16_t vid = 0;
    uint16_t pid = 0;

    GetUSBInfo(path_file, "cdc_acm", "tty", "ttyACM", vid, pid, bus, name);

    serialList.emplace_back(path, name, bus, vid, pid);
  }
  return serialList;
}

}  // namespace zebral

#endif  //__linux__
