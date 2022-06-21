/// \file platform.cpp
/// Platform object (for app/thread initialization for platforms) and
/// utility functions for common tasks that differ between platforms.
#include "platform.hpp"
#include <codecvt>
#include <fstream>
#include <iomanip>
#include <locale>
#include <regex>
#include <string>
#include "errors.hpp"
#include "find_files.hpp"

#if _WIN32
#include <winrt/base.h>
#endif

namespace zebral
{
Platform::~Platform()
{
#if _WIN32
  winrt::uninit_apartment();
#endif
}

Platform::Platform()
{
  SetUnhandled();

#if _WIN32
  winrt::init_apartment();
#endif
}

double zba_elapsed_sec(ZBA_TSTAMP start)
{
  return zba_elapsed_sec(start, zba_now());
}

double zba_elapsed_sec(ZBA_TSTAMP start, ZBA_TSTAMP end)
{
  double umsec = static_cast<double>(
      std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
  return umsec / 1000000.0;
}

ZBA_CLOCK::time_point zba_now()
{
  return ZBA_CLOCK::now();
}

std::string zba_local_time(ZBA_TSTAMP tp, int sec_precision)
{
  std::time_t t = ZBA_CLOCK::to_time_t(tp);
  struct tm buf;
  ZBA_LOCALTIME(&t, &buf);

  double epoch_sec      = zba_elapsed_sec(ZBA_TSTAMP());
  double fractional_sec = epoch_sec - static_cast<int>(epoch_sec);

  std::stringstream ss;
  ss << std::put_time(&buf, "%Y-%m-%d_%H-%M-%S");

  // Add subsecond precision if set
  if (sec_precision)
  {
    std::stringstream ss_sec;
    ss_sec << std::setprecision(sec_precision) << fractional_sec;

    // remove the leading '0',keep up to our desired length.
    // note: on windows, especially with very low values, sometimes got LONGER than the set
    // precision?
    std::string fract_out = ss_sec.str().substr(1, sec_precision + 1);
    // add any trailing '0'
    while (fract_out.length() < static_cast<size_t>(sec_precision + 1)) fract_out += "0";

    ss << fract_out;
  }
  return ss.str();
}

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
