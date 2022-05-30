#include "camera_info.hpp"

namespace zebral
{
std::ostream& operator<<(std::ostream& os, const CameraInfo& camInfo)
{
  os << "Camera " << camInfo.index << ": API(" << camInfo.api << ") [" << camInfo.name << "]"
     << std::endl;
  os << "    vid:" << std::hex << camInfo.vid << " pid:" << std::hex << camInfo.pid << std::dec
     << std::endl;
  os << "    <" << camInfo.path << ">" << std::endl;

  for (const auto& curFormat : camInfo.formats)
  {
    os << "   (" << curFormat.width << "," << curFormat.height << ") Ch: " << curFormat.channels
       << " Bits:" << curFormat.bitdepth << " Format:" << std::hex << curFormat.format << std::dec
       << std::endl;
  }
  return os;
}

}  // namespace zebral