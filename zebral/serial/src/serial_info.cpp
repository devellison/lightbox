/// \file serial_info.cpp
/// Serial device information implementation
#include "serial_info.hpp"
#include "log.hpp"
#include "platform.hpp"

namespace zebral
{
bool SerialInfo::operator<(const SerialInfo& s) const
{
  if (path < s.path)
    return true;
  else if (path > s.path)
    return false;

  if (bus < s.bus)
    return true;
  else if (bus > s.bus)
    return false;

  if (device_name < s.device_name)
    return true;
  else if (device_name > s.device_name)
    return false;

  if (vid < s.vid)
    return true;
  else if (vid > s.vid)
    return false;

  if (pid < s.pid)
    return true;
  else if (pid > s.pid)
    return false;

  return false;
}

std::ostream& operator<<(std::ostream& os, const SerialInfo& s)
{
  std::string deviceOut = zba_format("{} [{}]", s.device_name, s.path);

#if __linux__
  // Bus is fugly on windows
  deviceOut += zba_format(" <{}>", s.bus);
#endif

  if (s.vid != 0)
  {
    deviceOut += zba_format(" ({:x}:{:x})", s.vid, s.pid);
  }
  os << deviceOut << std::endl;
  return os;
}
}  // namespace zebral