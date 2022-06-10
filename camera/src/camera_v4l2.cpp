#if __linux__
#include "camera_v4l2.hpp"

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
#include "find_files.hpp"
#include "log.hpp"
#include "platform.hpp"

namespace zebral
{
/// Retry interrupted ioctls
int ioctl_retry(int fd, long unsigned int request, void* param)
{
  int r = -1;
  do
  {
    r = ioctl(fd, request, param);
  } while ((-1 == r) && (EINTR == errno));
  return r;
}

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
  impl_->handle_.reset(open(info_.path.c_str(), O_RDWR));

  if (impl_->handle_.bad())
  {
    ZBA_THROW("Error opening device(): " + info_.path, Result::ZBA_CAMERA_OPEN_FAILED);
  }

  v4l2_capability caps;
  memset(&caps, 0, sizeof(caps));
  if (-1 == ioctl_retry(impl_->handle_, VIDIOC_QUERYCAP, &caps))
  {
    ZBA_THROW("Error querying device: " + info_.name, Result::ZBA_CAMERA_OPEN_FAILED);
  }

  if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
  {
    ZBA_THROW("Capture interface not supported: " + info_.name, Result::ZBA_CAMERA_OPEN_FAILED);
  }

  struct v4l2_fmtdesc formatDesc;
  v4l2_frmsizeenum frameSize;

  for (int format_idx = 0;; format_idx++)
  {
    memset(&formatDesc, 0, sizeof(formatDesc));
    formatDesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    formatDesc.index = format_idx;

    if (-1 == ioctl_retry(impl_->handle_, VIDIOC_ENUM_FMT, &formatDesc))
    {
      ZBA_ERRNO("Error enumerating video formats.");
      // Done.
      break;
    }
    for (int fsize_idx = 0;; fsize_idx++)
    {
      memset(&frameSize, 0, sizeof(frameSize));
      frameSize.index        = fsize_idx;
      frameSize.pixel_format = formatDesc.pixelformat;
      if (-1 == ioctl_retry(impl_->handle_, VIDIOC_ENUM_FRAMESIZES, &frameSize))
      {
        break;
      }
      std::string format(reinterpret_cast<char*>(&frameSize.pixel_format), 4);
      float fps = 0.0f;
      /// {TODO} investigate stepwise sizes. For now, ignore them.
      if (V4L2_FRMSIZE_TYPE_DISCRETE == frameSize.type)
      {
        int width     = frameSize.discrete.width;
        int height    = frameSize.discrete.height;
        int channels  = 0;
        int bytespppc = 0;

        FormatInfo fmt(width, height, fps, channels, bytespppc, format);
        if (IsFormatSupported(fmt))
        {
          info_.AddFormat(fmt);
        }
      }
    }
  }
}

CameraV4L2::~CameraV4L2() {}

void CameraV4L2::OnStart() {}

void CameraV4L2::OnStop() {}

std::vector<CameraInfo> CameraV4L2::Enumerate()
{
  std::vector<CameraInfo> cameras;

  auto video_devs = FindFiles("/dev/", "video([0-9]+)");
  for (auto curMatch : video_devs)
  {
    int idx = std::stoi(curMatch.matches[1]);

    std::string path = curMatch.dir_entry.path().string();

    ZBA_LOG("Checking {}", path.c_str());

    v4l2_capability caps;
    memset(&caps, 0, sizeof(caps));
    {
      AutoClose handle(open(path.c_str(), O_RDONLY));
      if (handle.bad()) continue;

      // check if it's v4l2...
      int result = ioctl_retry(handle, VIDIOC_QUERYCAP, &caps);
      if (-1 == result) continue;
    }
    if (!(V4L2_CAP_VIDEO_CAPTURE & caps.capabilities)) continue;

    std::string name(reinterpret_cast<char*>(&caps.card[0]));
    std::string driver(reinterpret_cast<char*>(&caps.driver[0]));
    std::string bus(reinterpret_cast<char*>(&caps.bus_info[0]));

    uint16_t vid = 0;
    uint16_t pid = 0;
    if (bus.compare(0, 3, "usb") == 0)
    {
      /// {TODO} It's a USB device. Track down the VID/PID
      // / sys / bus / usb / drivers / uvcvideo
    }
    cameras.emplace_back(idx, 0, name, bus, path, driver, vid, pid);
  }

  return cameras;
}

FormatInfo CameraV4L2::OnSetFormat(const FormatInfo&)  // info)
{
  FormatInfo fmt;
  return fmt;
}

}  // namespace zebral
#endif  // __linux__