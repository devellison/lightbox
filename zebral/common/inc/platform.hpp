/// \file platform.hpp
/// Abstractions of platform-specific stuff.
#ifndef LIGHTBOX_PLATFORM_HPP_
#define LIGHTBOX_PLATFORM_HPP_

#include <chrono>
#include <ctime>
#include <string>

#if _WIN32
#include <windows.h>
#include <winrt/base.h>
#include <format>
#include <source_location>
#define zba_format     std::format
#define zba_vformat    std::vformat
#define zba_make_args  std::make_format_args
#define zba_source_loc std::source_location
#elif __linux__
#include <fmt/format.h>
#define zba_format     fmt::format
#define zba_vformat    fmt::vformat
#define zba_make_args  fmt::make_format_args
#define zba_source_loc zebral::source_location
#endif

namespace zebral
{
/// Platform object - performs initializations for the platform.
/// Generally used per-thread for stuff like CoInitializeEx.
class Platform
{
 public:
  /// Constructor. Performs per-thread initializations like CoInitializeEx.
  Platform();

  /// Destructor. Releases anything from constructor.
  ~Platform();
};

// Time utilities
#if _WIN32
// macro to make the MSVC localtime_s look like std::localtime_s
#define ZBA_LOCALTIME(in_timet, out_buf) localtime_s(out_buf, in_timet);
#else
#define ZBA_LOCALTIME(in_timet, out_buf) localtime_r(in_timet, out_buf)
#endif

using ZBA_CLOCK  = std::chrono::system_clock;  ///< Clock we're using
using ZBA_TSTAMP = ZBA_CLOCK::time_point;      ///< Timestamp for that clock

/// Gets the current time
/// \returns ZBA_TIMESTAMP (std::chrono::system_clock::time_point) of current time.
ZBA_TSTAMP zba_now();

/// Gets elapsed seconds from start to now.
/// \param start - timestamp of start of duration
/// \returns double - number of seconds between start and now
double zba_elapsed_sec(ZBA_TSTAMP start);

/// Gets elapsed seconds from start to end
/// \param start - timestamp of start of duration
/// \param end - timestamp of end of duration
/// \returns double - number of seconds between start and end.
double zba_elapsed_sec(ZBA_TSTAMP start, ZBA_TSTAMP end);

/// Converts a timestamp to a localtime string Y-M-D_H-M-S.s
/// If sec_precision is left off, does not have ".s".
/// Otherwise gives sec_precision digits of precision
/// \param tp - timestamp
/// \param sec_precision - number of digits for fractional seconds.
/// \return std::string - formatted local time string
std::string zba_local_time(ZBA_TSTAMP tp, int sec_precision = 0);

#ifndef _WIN32
/// source_location replacement until people have it in c++20
/// (missing from clang/gcc that I can get on ubuntu 20)
struct source_location
{
  constexpr source_location(const char* file, const char* function, const int line)
      : file_(file),
        function_(function),
        line_(line)
  {
  }
  static constexpr source_location current(const char* file     = __builtin_FILE(),
                                           const char* function = __builtin_FUNCTION(),
                                           const int line       = __builtin_LINE())
  {
    return source_location(file, function, line);
  };

  constexpr const char* file_name() const
  {
    return file_;
  }

  constexpr const char* function_name() const
  {
    return function_;
  }

  constexpr int line() const
  {
    return line_;
  }

  const char* file_;
  const char* function_;
  const int line_;
};
#endif

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

#endif  // LIGHTBOX_PLATFORM_HPP_
