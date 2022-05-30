#include "camera_info.hpp"

namespace zebral
{
std::ostream& operator<<(std::ostream& os, const CameraInfo& camInfo)
{
  os << "Camera " << camInfo.index << ": Api(" << camInfo.api << ")";
  return os;
}

}  // namespace zebral