/// \file serial_info.hpp
/// Info struct about serial devices
#ifndef LIGHTBOX_SERIAL_SERIAL_INFO_HPP_
#define LIGHTBOX_SERIAL_SERIAL_INFO_HPP_
#include <string>

namespace zebral
{
/// Serial Device information
struct SerialInfo
{
 public:
  SerialInfo(const std::string& s_path, const std::string& s_name, const std::string& s_bus,
             uint16_t s_vid = 0, uint16_t s_pid = 0)
      : path(s_path),
        device_name(s_name),
        bus(s_bus),
        vid(s_vid),
        pid(s_pid)
  {
  }

  /// Less-than for sorting
  bool operator<(const SerialInfo& s) const;

  std::string path;         ///< File path of device (e.g. /dev/ttyACM0 or COM4)
  std::string device_name;  ///< Friendly name
  std::string bus;          ///< Bus path
  uint16_t vid;             ///< Vendor ID for USB devices
  uint16_t pid;             ///< Product ID for USB devices
};

/// Debugging dump
std::ostream& operator<<(std::ostream& os, const SerialInfo& s);
}  // namespace zebral
#endif  // LIGHTBOX_SERIAL_SERIAL_INFO_HPP_
