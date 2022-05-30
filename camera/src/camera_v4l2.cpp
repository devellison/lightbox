#ifdef __linux__

#include "camera_v4l2.hpp"
#include "errors.hpp"
#include "log.hpp"
#include "platform.hpp"

namespace zebral
{
CameraV4L2::CameraV4L2(const CameraInfo &info) : Camera(info) {}

CameraV4L2::~CameraV4L2() {}

void CameraV4L2::OnStart() {}

void CameraV4L2::OnStop() {}

}  // namespace zebral

#endif  // __linux__