#if _WIN32
#include "serial_line_channel.hpp"
#include "errors.hpp"
#include "log.hpp"
#include "platform.hpp"

#include <winrt/base.h>
#include <winrt/windows.devices.enumeration.h>
#include <winrt/windows.devices.serialcommunication.h>
#include <winrt/windows.storage.streams.h>
#include <winrt/windows.system.h>

using namespace winrt;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::SerialCommunication;
using namespace Windows::Storage::Streams;
using namespace Windows::System;

namespace zebral
{
SerialLineChannel::SerialLineChannel(const std::string& path) : path_(path) {}
SerialLineChannel::~SerialLineChannel() {}

std::vector<std::string> SerialLineChannel::Enumerate()
{
  std::vector<std::string> deviceList;

  string aqs                 = SerialDevice.GetDeviceSelector();
  var deviceCollection       = await DeviceInformation.FindAllAsync(aqs);
  List<string> portNamesList = new List<string>();
  foreach (var item in deviceCollection)
  {
    var serialDevice = await SerialDevice.FromIdAsync(item.Id);
    var portName     = serialDevice.PortName;
    portNamesList.Add(portName);
  }

  return deviceList;
}

}  // namespace zebral
#endif  // _WIN32