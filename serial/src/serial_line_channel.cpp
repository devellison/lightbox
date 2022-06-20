#if _WIN32
#include "serial_line_channel.hpp"
#include "errors.hpp"
#include "log.hpp"
#include "platform.hpp"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/windows.devices.enumeration.h>
#include <winrt/windows.devices.h>
#include <winrt/windows.devices.serialcommunication.h>
#include <winrt/windows.storage.streams.h>
#include <winrt/windows.system.h>

#include <winrt/base.h>

using namespace winrt;
using namespace Windows::Devices;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::SerialCommunication;
using namespace Windows::Storage::Streams;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

namespace zebral
{
SerialLineChannel::SerialLineChannel(const std::string& path) : path_(path) {}
SerialLineChannel::~SerialLineChannel() {}

std::map<std::string, std::string> SerialLineChannel::Enumerate()
{
  std::map<std::string, std::string> deviceList;

  winrt::hstring serial_selector = SerialDevice::GetDeviceSelector();

  auto devCol = DeviceInformation::FindAllAsync(serial_selector).get();
  for (const auto& device : devCol)
  {
    auto devInfo              = DeviceInformation::CreateFromIdAsync(device.Id()).get();
    winrt::hstring deviceName = devInfo.Name();
    auto serialDevice         = SerialDevice::FromIdAsync(device.Id()).get();
    winrt::hstring portName   = serialDevice.PortName();
    deviceList.emplace(std::make_pair(winrt::to_string(deviceName), winrt::to_string(portName)));
  }

  return deviceList;
}

}  // namespace zebral
#endif  // _WIN32