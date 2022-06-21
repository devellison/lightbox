/// \file serial_channel.hpp
/// Simple line-based serial communications.
#ifndef LIGHTBOX_SERIAL_SERIAL_CHANNEL_HPP_
#define LIGHTBOX_SERIAL_SERIAL_CHANNEL_HPP_

#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "serial_info.hpp"

namespace zebral
{
/// Serial line communications class
class SerialChannel
{
 public:
  /// construct and open a serial device from an info.
  SerialChannel(const SerialInfo& info);

  /// close serial device in dtor
  ~SerialChannel();

  /// Retrieve a list of current devices
  /// \returns std::vector<SerialInfo> List of filled SerialInfo structs
  static std::vector<SerialInfo> Enumerate();

 protected:
  SerialInfo info_;           ///< Info about the device
  std::thread read_thread_;   ///< Reader thread
  mutable std::mutex mutex_;  ///< Mutex to lock device access / internals
};

}  // namespace zebral
#endif  // LIGHTBOX_SERIAL_SERIAL_CHANNEL_HPP_
