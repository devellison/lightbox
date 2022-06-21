#if _WIN32
#include "errors.hpp"
#include "log.hpp"
#include "platform.hpp"
#include "serial_channel.hpp"

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
SerialChannel::SerialChannel(const SerialInfo& info) : info_(info) {}
SerialChannel::~SerialChannel() {}

std::vector<SerialInfo> SerialChannel::Enumerate()
{
  std::vector<SerialInfo> deviceList;

  winrt::hstring serial_selector = SerialDevice::GetDeviceSelector();

  auto devCol = DeviceInformation::FindAllAsync(serial_selector).get();
  for (const auto& device : devCol)
  {
    auto devInfo           = DeviceInformation::CreateFromIdAsync(device.Id()).get();
    std::string deviceName = winrt::to_string(devInfo.Name());

    auto serialDevice    = SerialDevice::FromIdAsync(device.Id()).get();
    std::string portName = winrt::to_string(serialDevice.PortName());

    // {TODO} fill these out as well
    std::string bus = winrt::to_string(device.Id());
    uint16_t vid    = serialDevice.UsbVendorId();
    uint16_t pid    = serialDevice.UsbProductId();

    deviceList.emplace_back(portName, deviceName, bus, vid, pid);
  }

  return deviceList;
}

}  // namespace zebral
#endif  // _WIN32