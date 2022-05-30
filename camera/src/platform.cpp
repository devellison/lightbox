#include "platform.hpp"
#include <iomanip>
#include "errors.hpp"

#if _WIN32
#include <mfapi.h>
#include <windows.h>
#endif

namespace zebral
{
Platform::~Platform()
{
#if _WIN32
  MFShutdown();
  CoUninitialize();
#endif
}

Platform::Platform()
{
  SetUnhandled();
#if _WIN32
  if FAILED (CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))
  {
    ZEBRAL_THROW("COM initialization failed.", Result::ZBA_SYS_COM_ERROR);
  }

  if (FAILED(MFStartup(MF_VERSION)))
  {
    ZEBRAL_THROW("Media Foundation initialization failed.", Result::ZBA_SYS_MF_ERROR);
  }

#endif
}

double zebral_elapsed_sec(ZEBRAL_TSTAMP start)
{
  return zebral_elapsed_sec(start, zebral_now());
}

double zebral_elapsed_sec(ZEBRAL_TSTAMP start, ZEBRAL_TSTAMP end)
{
  double umsec = static_cast<double>(
      std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
  return umsec / 1000000.0;
}

ZEBRAL_CLOCK::time_point zebral_now()
{
  return ZEBRAL_CLOCK::now();
}

std::string zebral_local_time(ZEBRAL_TSTAMP tp, int sec_precision)
{
  std::time_t t = ZEBRAL_CLOCK::to_time_t(tp);
  struct tm buf;
  ZEBRAL_LOCALTIME(&t, &buf);

  double epoch_sec      = zebral_elapsed_sec(ZEBRAL_TSTAMP());
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

std::string WideToString(const WCHAR* wide)
{
#if _WIN32
  // IIRC this is probably quite slow. If we need to use it often write a custom converter or find
  // one.
  std::string value;
  int strLength = WideCharToMultiByte(CP_UTF8, 0, wide, -1, 0, 0, 0, 0);
  value.resize(strLength);
  WideCharToMultiByte(CP_UTF8, 0, wide, -1, static_cast<LPSTR>(value.data()), strLength, 0, 0);
  value.resize(strLength - 1);
  return value;
#else
  ZEBRAL_ASSERT(_WIN32, "Not implemented on other platforms.");
#endif

  /// Deprecated? Really?
  /// implement if we need to.
#if 0
  std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_val;
  return utf8_val.str();
#endif
}

}  // namespace zebral