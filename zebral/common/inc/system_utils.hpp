/// \file system_utils.hpp
/// System utility functions
#ifndef LIGHTBOX_ZEBRAL_COMMON_SYSTEM_UTILS_HPP_
#define LIGHTBOX_ZEBRAL_COMMON_SYSTEM_UTILS_HPP_

#include "platform.hpp"

namespace zebral
{
#if __linux__
/// Retrieves USB information for a device given the device type info
/// \param device_file - filename of the device, e.g. "video0" or "ttyACM0"
/// \param driver_type - type of driver, e.g. "cdc_acm" or "uvcvideo"
/// \param deviceType - type of device, e.g. "tty" or "video4linux"
/// \param devicePrefix - prefix used for devices, e.g. "video" or "ttyACM"
/// \param vid - receives vendor id
/// \param pid - receives product id
/// \param bus - receives usb bus path
/// \param name - receives usb product name
/// \returns true if pid/vid successful
bool GetUSBInfo(const std::string& device_file, const std::string& driverType,
                const std::string& deviceType, const std::string& devicePrefix, uint16_t& vid,
                uint16_t& pid, std::string& bus, std::string& name);
#endif  // __linux__

}  // namespace zebral

#endif  // LIGHTBOX_ZEBRAL_COMMON_SYSTEM_UTILS_HPP_