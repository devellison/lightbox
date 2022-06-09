#include "camera_info.hpp"
#include <cmath>
#include <limits>

namespace zebral
{
std::ostream& operator<<(std::ostream& os, const CameraInfo& camInfo)
{
  os << "Camera " << camInfo.index << ": API(" << camInfo.api << ") [" << camInfo.name << "]"
     << std::endl;
  os << "    path:" << camInfo.path << " bus:" << camInfo.bus << " driver:" << camInfo.driver
     << std::endl;
  os << "    vid:" << std::hex << camInfo.vid << " pid:" << std::hex << camInfo.pid << std::dec
     << std::endl;
  os << "    <" << camInfo.path << ">" << std::endl;
#define ENUM_FORMATS 1
#if ENUM_FORMATS
  int i = 0;
  for (auto const& curFormat : camInfo.formats)
  {
    os << i << ": " << curFormat << std::endl;
    ++i;
  }
#endif
  return os;
}

std::ostream& operator<<(std::ostream& os, const FormatInfo& fmtInfo)
{
  os << "(" << fmtInfo.width << ", " << fmtInfo.height << ") " << fmtInfo.format;
  if (fmtInfo.fps > std::numeric_limits<float>::epsilon())
  {
    os << " @" << fmtInfo.fps << "fps";
  }
  return os;
}

bool FormatInfo::operator<(const FormatInfo& f) const
{
  if (width < f.width)
    return false;
  else if (width > f.width)
    return true;

  if (height < f.height)
    return false;
  else if (height > f.height)
    return true;

  if (fps < f.fps)
    return false;
  else if (fps > f.fps)
    return true;

  if (channels < f.channels)
    return false;
  else if (channels > f.channels)
    return true;

  if (bytespppc < f.bytespppc)
    return false;
  else if (bytespppc > f.bytespppc)
    return true;

  if (format < f.format)
    return false;
  else if (format > f.format)
    return true;

  return false;
}

bool FormatInfo::Matches(const FormatInfo& f) const
{
  if ((width != f.width) && (width) && (f.width)) return false;
  if ((height != f.height) && (height) && (f.height)) return false;
  if ((channels != f.channels) && (channels) && (f.channels)) return false;
  if ((format != f.format) && (!format.empty()) && (!f.format.empty())) return false;
  // 0.1 because 29.97 and 30, but calculated value.
  if ((std::abs(fps - f.fps) >= 0.1) && (fps > std::numeric_limits<float>::epsilon()) &&
      (f.fps > std::numeric_limits<float>::epsilon()))
    return false;
  return true;
}

}  // namespace zebral
