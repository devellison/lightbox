/// \file system_utils.cpp
/// System utility functions
#include "system_utils.hpp"

#include <filesystem>
#include <fstream>

#include "errors.hpp"
#include "find_files.hpp"
#include "log.hpp"

namespace zebral
{
#if __linux__
bool GetUSBInfo(const std::string& device_file, const std::string& driverType,
                const std::string& deviceType, const std::string& devicePrefix, uint16_t& vid,
                uint16_t& pid, std::string& bus, std::string& name)
{
  /// {TODO} It's a USB device. Track down the VID/PID
  // / sys / bus / usb / drivers / uvcvideo
  std::string driverSearch = zba_format("/sys/bus/usb/drivers/{}/", driverType);
  std::string devSearch    = zba_format("^{}([0-9]*)$", devicePrefix);

  auto usbDevAddrs = FindFiles(driverSearch, "^([0-9-.]*):([0-9-.]*)$");
  for (auto& curDevAddr : usbDevAddrs)
  {
    if (!std::filesystem::exists(curDevAddr.dir_entry.path() / deviceType))
    {
      continue;
    }

    auto usbVideoNames = FindFiles(curDevAddr.dir_entry.path() / deviceType, devSearch);
    for (auto curVideo : usbVideoNames)
    {
      if (device_file == curVideo.dir_entry.path().filename().string())
      {
        // Grab idProduct and idVendor files
        bus = curDevAddr.matches[1];

        std::ifstream nameFile(
            zba_format("/sys/bus/usb/drivers/usb/{}/product", curDevAddr.matches[1]));

        if (nameFile.is_open())
        {
          std::getline(nameFile, name);
        }

        std::ifstream prodFile(
            zba_format("/sys/bus/usb/drivers/usb/{}/idProduct", curDevAddr.matches[1]));
        std::ifstream vendFile(
            zba_format("/sys/bus/usb/drivers/usb/{}/idVendor", curDevAddr.matches[1]));
        if (prodFile.is_open() && vendFile.is_open())
        {
          std::string prodStr, vendStr;
          std::getline(prodFile, prodStr);
          std::getline(vendFile, vendStr);
          vid = std::stol(vendStr, 0, 16);
          pid = std::stol(prodStr, 0, 16);
          return true;
        }
        break;
      }
    }
  }
  return false;
}

#endif  // __linux__

}  // namespace zebral
