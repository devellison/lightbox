#include "camera_info.hpp"
#include <cmath>
#include <limits>
#include "errors.hpp"
#include "log.hpp"

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

uint32_t FourCCToUInt32(const std::string& fmt_format)
{
  if (fmt_format.empty()) return 0;
  ZBA_ASSERT(fmt_format.length() == 4, "Invalid frame format string.");
  return *reinterpret_cast<const uint32_t*>(fmt_format.data());
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
    case FOURCCTOUINT32("Z16 "):
    case FOURCCTOUINT32("GREY"):
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
    case FOURCCTOUINT32("Z16 "):
      return 2;
    case FOURCCTOUINT32("GREY"):
      return 1;
    case 0:
      return 0;
    default:
      // ZBA_LOG("Unsupported format");
      return 0;
  }
}

}  // namespace zebral
