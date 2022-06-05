/// \file platform.hpp
/// Abstractions of platform-specific stuff.
#ifndef LIGHTBOX_PLATFORM_HPP_
#define LIGHTBOX_PLATFORM_HPP_

#include <chrono>
#include <string>

#if _WIN32
#include <windows.h>
#include <winrt/base.h>
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
#define ZBA_LOCALTIME(int_timet, out_buf) std::localtime_s(in_timet, out_buf)
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

#if _WIN32
/// Convert wide char to a utf-8 std::string
std::string WideToString(const WCHAR* wide);

/// Convert std::string to UCS2 in a winrt hstring for use in winrt.
winrt::hstring StringToWide(const std::string& slim);

#endif  // _WIN32

}  // namespace zebral

#endif  // LIGHTBOX_PLATFORM_HPP_
