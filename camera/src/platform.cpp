#include "platform.hpp"
#include <codecvt>
#include <iomanip>
#include <locale>
#include <string>
#include "errors.hpp"

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
  // SetUnhandled();

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
    while (fract_out.length() < sec_precision + 1) fract_out += "0";

    ss << fract_out;
  }
  return ss.str();
}
#if _WIN32
std::string WideToString(const WCHAR* wide)
{
  // IIRC this is probably quite slow. If we need to use it often write a custom converter or find
  // one.
  DWORD strLength = WideCharToMultiByte(CP_UTF8, 0, wide, -1, 0, 0, 0, 0);
  std::string value;
  value.resize(strLength);
  WideCharToMultiByte(CP_UTF8, 0, wide, -1, static_cast<LPSTR>(value.data()), strLength, 0, 0);
  value.resize(strLength - 1);
  return value;
}

winrt::hstring StringToWide(const std::string& str)
{
  DWORD strLength = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
  std::wstring value;
  value.resize(strLength);
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, static_cast<LPWSTR>(value.data()), strLength);
  value.resize(strLength - 1);
  winrt::hstring res(value.c_str());
  return res;
}
#endif  //_WIN32

}  // namespace zebral