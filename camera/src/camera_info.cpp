/// \file camera_info.cpp
/// Implementation of CameraInfo, FormatInfo, and related functions to describe camera devices.

#include "camera_info.hpp"
#include <cmath>
#include <limits>
#include "errors.hpp"
#include "log.hpp"

namespace zebral
{
std::ostream& operator<<(std::ostream& os, const CameraInfo& camInfo)
{
  os << "Camera " << camInfo.index << ": " << camInfo.name << std::endl;
  os << "    path: [" << camInfo.path << "] bus:<" << camInfo.bus << "> driver:" << camInfo.driver
     << std::endl;
  os << "    vid:pid: (" << std::hex << camInfo.vid << ":" << std::hex << camInfo.pid << std::dec
     << ")" << std::endl;
#define ENUM_FORMATS 0
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

uint32_t FourCCToUInt32(const std::string& fmt_format)
{
  if (fmt_format.empty()) return 0;

  // {TODO} OK, sometimes these are GUIDs... we'll want to add support for those later.
  if (fmt_format.length() > 4)
  {
    ZBA_LOG("Invalid frame format string: {}", fmt_format);
    return 0;
  }

  // Linux space-pads, but Windows doesn't.
  auto tmpFmt = fmt_format;
  while (tmpFmt.length() < 4)
  {
    tmpFmt.append(" ");
  }
  return *reinterpret_cast<const uint32_t*>(tmpFmt.data());
}

int ChannelsFromFourCC(const std::string& fmt_format)
{
  switch (FourCCToUInt32(fmt_format))
  {
#if _WIN32
    case FOURCCTOUINT32("YUY2"):
#else
    case FOURCCTOUINT32("YUYV"):
#endif
      return 3;
    case FOURCCTOUINT32("NV12"):
      return 3;
    case FOURCCTOUINT32("RGB "):
    case FOURCCTOUINT32("BGR "):
      return 3;
    case FOURCCTOUINT32("RGBA"):
    case FOURCCTOUINT32("BGRA"):
    case FOURCCTOUINT32("RGBT"):
    case FOURCCTOUINT32("BGRT"):
      return 4;
    case FOURCCTOUINT32("D16 "):  // Windows Depth
    case FOURCCTOUINT32("L8  "):  // Windows IR (?)
    case FOURCCTOUINT32("Z16 "):  // Linux Depth
    case FOURCCTOUINT32("GREY"):  // Linux IR
      return 1;
    case 0:
      return 0;
    default:
      // ZBA_LOG("Unsupported format");
      return 0;
  }
}

int BytesPPPCFromFourCC(const std::string& fmt_format)
{
  switch (FourCCToUInt32(fmt_format))
  {
#if _WIN32
    case FOURCCTOUINT32("YUY2"):
#else
    case FOURCCTOUINT32("YUYV"):
#endif
      return 1;
    case FOURCCTOUINT32("NV12"):
      return 1;
    case FOURCCTOUINT32("RGB "):
    case FOURCCTOUINT32("BGR "):
      return 1;
    case FOURCCTOUINT32("RGBA"):
    case FOURCCTOUINT32("BGRA"):
    case FOURCCTOUINT32("RGBT"):
    case FOURCCTOUINT32("BGRT"):
      return 1;
    case FOURCCTOUINT32("Z16 "):  // Linux
    case FOURCCTOUINT32("D16 "):  // Windows
      return 2;
    case FOURCCTOUINT32("GREY"):  // Linux
    case FOURCCTOUINT32("L8  "):  // Windows
      return 1;
    case 0:
      return 0;
    default:
      // ZBA_LOG("Unsupported format");
      return 0;
  }
}

}  // namespace zebral
