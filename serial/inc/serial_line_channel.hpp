/// \file serial_line_channel.hpp
/// Simple line-based serial communications.
#ifndef LIGHTBOX_SERIAL_SERIAL_LINE_CHANNEL_HPP_
#define LIGHTBOX_SERIAL_SERIAL_LINE_CHANNEL_HPP_

#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

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

/// Serial line communications class
class SerialLineChannel
{
 public:
  /// construct and open a serial device from an info.
  SerialLineChannel(const SerialInfo& info);

  /// close serial device in dtor
  ~SerialLineChannel();

  /// Retrieve a list of current devices
  /// \returns std::vector<SerialInfo> List of filled SerialInfo structs
  static std::vector<SerialInfo> Enumerate();

 protected:
  SerialInfo info_;           ///< Info about the device
  std::thread read_thread_;   ///< Reader thread
  mutable std::mutex mutex_;  ///< Mutex to lock device access / internals
};

}  // namespace zebral
#endif  // LIGHTBOX_SERIAL_SERIAL_LINE_CHANNEL_HPP_
