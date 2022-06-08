#if __linux__
#include "camera_v4l2.hpp"

#include <filesystem>
#include <functional>
#include <regex>
#include <string>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/videodev2.h>

#include "auto_close.hpp"
#include "errors.hpp"
#include "log.hpp"
#include "platform.hpp"

namespace zebral
{
/// Fun implementation details.
class CameraV4L2::Impl
{
 public:
  Impl(CameraV4L2* parent) : parent_(*parent), started_(false) {}

  CameraV4L2& parent_;
  AutoClose handle_;
  bool started_;  ///< True if started.
};

CameraV4L2::CameraV4L2(const CameraInfo& info) : Camera(info)
{
  impl_ = std::make_unique<Impl>(this);
  impl_->handle_.reset(open(info_.path.c_str(), O_RDONLY));

  if (impl_->handle_.bad())
  {
    ZBA_THROW("Error opening device: " + info_.path, Result::ZBA_CAMERA_OPEN_FAILED);
  }
}

CameraV4L2::~CameraV4L2() {}

void CameraV4L2::OnStart() {}

void CameraV4L2::OnStop() {}

std::vector<CameraInfo> CameraV4L2::Enumerate()
{
  std::vector<CameraInfo> cameras;
  for (const std::filesystem::directory_entry& dir_entry :
       std::filesystem::directory_iterator{"/dev/"})
  {
    // Does it start with "video"?
    std::string filename = dir_entry.path().filename();

    if (filename.compare(0, 5, "video") != 0) continue;

    // open it
    std::string path = dir_entry.path().string();
    AutoClose handle(open(path.c_str(), O_RDONLY));
    if (handle.bad()) continue;

    // check if it's v4l2...
    v4l2_capability caps;
    int result = ioctl(handle.get(), VIDIOC_QUERYCAP, &caps);
    if (-1 == result) continue;

    if (!(V4L2_CAP_VIDEO_CAPTURE & caps.capabilities)) continue;

// Looks like these are straight ASCIIZ, no sizeof required.
#define FIELD_TO_STRING(x) reinterpret_cast<char*>(&x[0])

    std::string name(FIELD_TO_STRING(caps.card));
    std::string driver(FIELD_TO_STRING(caps.driver));
    std::string bus(FIELD_TO_STRING(caps.bus_info));

    int idx      = std::atoi(filename.substr(5, std::string::npos).c_str());
    uint16_t vid = 0;
    uint16_t pid = 0;
    if (bus.compare(0, 3, "usb") == 0)
    {
      /// {TODO} It's a USB device. Track down the VID/PID
    }
    cameras.emplace_back(idx, 0, name, bus, path, driver, vid, pid);
  }

  return cameras;
}

void CameraV4L2::OnSetFormat(const FormatInfo&)  // info)
{
}

}  // namespace zebral
#endif  // __linux__