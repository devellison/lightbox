/// \file camera_v4l2.cpp
/// CameraPlatform implementation for Linux / V4L2
#if __linux__
#include "camera_platform.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <functional>
#include <memory>
#include <regex>
#include <string>
#include <thread>

#include <linux/videodev2.h>
#include <sys/mman.h>

#include "buffer_memmap.hpp"
#include "convert.hpp"
#include "device_v4l2.hpp"
#include "errors.hpp"
#include "find_files.hpp"
#include "log.hpp"
#include "param.hpp"
#include "platform.hpp"
#include "system_utils.hpp"

namespace zebral
{
/// CameraPlatform's implementation details
class CameraPlatform::Impl
{
 public:
  // Constructor = opens the device
  Impl(CameraPlatform* parent);
  ~Impl() = default;

  /// Callback for EnumerateModes
  /// Returning false stops the enumeration.
  typedef std::function<bool(const v4l2_fmtdesc&, const v4l2_frmsizeenum&,
                             const FormatInfo& fmt_info)>
      ModeCallback;

  /// Enumerated all modes available on current device,
  /// calling the callback for each one.
  void EnumerateModes(ModeCallback cb);

  /// Enumerate controls and set up enabled ones.
  void EnumerateControls();

  /// Thread that reads data from the camera and calls callbacks
  void CaptureThread();

  /// Starts the camera
  void Start();

  /// Stops the camera
  void Stop();

  void OnParamChanged(Param* param, bool raw_set, bool auto_mode);

  void AddParameter(const std::string& name, int ioctl_id);

  static constexpr int kNumBuffers = 1;   ///< Number of buffers to allocate
  std::unique_ptr<BufferGroup> buffers_;  ///< Buffer group for buffers
  CameraPlatform& parent_;                ///< Parent camera object
  DeviceV4L2Ptr device_;                  ///< Camera device file descriptor
  bool started_;                          ///< True if started.
  std::thread camera_thread_;

  std::mutex paramControlMutex_;
  std::map<std::string, int> paramControlMap_;
};

void CameraPlatform::Impl::EnumerateControls()
{
  v4l2_queryctrl queryctrl;
  memset(&queryctrl, 0, sizeof(queryctrl));
  queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;

  while (0 == device_->ioctl(VIDIOC_QUERYCTRL, &queryctrl))
  {
    std::string name = reinterpret_cast<const char*>(&queryctrl.name[0]);
    // If it's not disabled...
    if (!(queryctrl.flags & V4L2_CTRL_FLAG_DISABLED))
    {
      // Get the current value of the item
      double value = 0;
      v4l2_control control;
      memset(&control, 0, sizeof(control));
      control.id = queryctrl.id;
      if (0 == device_->ioctl(VIDIOC_G_CTRL, &control))
      {
        value = static_cast<double>(control.value);
      }
      {  // Map the name to the control id
        std::lock_guard<std::mutex> lock(paramControlMutex_);
        ZBA_LOG("Adding {}:{:x}", name, queryctrl.id);
        paramControlMap_.emplace(name, queryctrl.id);
      }

      // Create subscriber list
      ParamSubscribers callbacks{
          {name, std::bind(&CameraPlatform::Impl::OnParamChanged, this, std::placeholders::_1,
                           std::placeholders::_2, std::placeholders::_3)}};

      // If it's a menu item, create a menu parameter...
      if (V4L2_CTRL_TYPE_MENU == queryctrl.type)
      {
        struct v4l2_querymenu querymenu;
        memset(&querymenu, 0, sizeof(querymenu));
        querymenu.id = queryctrl.id;

        auto param = new ParamMenu(name, callbacks, value, queryctrl.default_value);

        for (auto idx = queryctrl.minimum; idx <= queryctrl.maximum; idx++)
        {
          querymenu.index = static_cast<unsigned int>(idx);
          if (0 == device_->ioctl(VIDIOC_QUERYMENU, &querymenu))
          {
            param->addValue(name, querymenu.index);
          }
        }
        {
          // Add to Camera's parameter list
          std::lock_guard<std::mutex> lock(parent_.parameter_mutex_);
          parent_.parameters_.emplace(name, std::shared_ptr<Param>(dynamic_cast<Param*>(param)));
        }
      }
      else
      {
        // Not a menu - ranged.  For now, translate everything to doubles,
        // but be sure to round when converting to value to send back.

        /// {TODO} Would be nice to support auto on/off like winrt does.
        /// Need to pair control ids somehow
        bool autoMode = false;
        auto param    = new ParamRanged<double, double>(
            name, callbacks, value, queryctrl.default_value, queryctrl.minimum, queryctrl.maximum,
            autoMode, RawToScaledNormal, ScaledToRawNormal);

        {  // Add to Camera's parameter list
          std::lock_guard<std::mutex> lock(parent_.parameter_mutex_);
          parent_.parameters_.emplace(name, std::shared_ptr<Param>(dynamic_cast<Param*>(param)));
        }
      }
    }
    // Go to next one.
    queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
  }
  if (EINVAL != errno)
  {
    ZBA_THROW_ERRNO("Error querying device for controls!", Result::ZBA_CAMERA_ERROR);
  }
}

CameraPlatform::CameraPlatform(const CameraInfo& info) : Camera(info)
{
  impl_ = std::make_unique<Impl>(this);
}

CameraPlatform::~CameraPlatform() {}

void CameraPlatform::OnStart()
{
  impl_->Start();
}

void CameraPlatform::OnStop()
{
  impl_->Stop();
}
std::vector<CameraInfo> CameraPlatform::Enumerate()
{
  std::vector<CameraInfo> cameras;

  auto video_devs = FindFiles("/dev/", "video([0-9]+)");
  for (auto curMatch : video_devs)
  {
    std::string path      = curMatch.dir_entry.path().string();
    std::string path_file = curMatch.dir_entry.path().filename().string();
    v4l2_capability caps;
    memset(&caps, 0, sizeof(caps));
    {
      DeviceV4L2 device(path);
      if (device.bad()) continue;

      // check if it's v4l2...
      int result = device.ioctl(VIDIOC_QUERYCAP, &caps);
      if (-1 == result) continue;
    }

    // Get device capabilities
    if (!(V4L2_CAP_VIDEO_CAPTURE & caps.capabilities)) continue;

    std::string name(reinterpret_cast<char*>(&caps.card[0]));
    std::string driver(reinterpret_cast<char*>(&caps.driver[0]));

    // Debating on bus - for serial devices we're just using the USB
    // path which is probably more useful to us if we want to select a
    // device based on where it's plugged in on USB, which is my usual
    // use-case.  I think for now we'll start with the caps bus, but
    // if it IS a usb device, switch to usb bus.
    std::string bus(reinterpret_cast<char*>(&caps.bus_info[0]));

    // We already have a friendly device name, but there's another in the
    // usb path
    std::string usbname;

    // VID/PID identify device type
    uint16_t vid = 0;
    uint16_t pid = 0;

    if (bus.compare(0, 3, "usb") == 0)
    {
      GetUSBInfo(path_file, "uvcvideo", "video4linux", "video", vid, pid, bus, usbname);
    }

    // For now we want to skip the metadata devices.
    struct v4l2_fmtdesc formatDesc;
    DeviceV4L2 device(path);
    memset(&formatDesc, 0, sizeof(formatDesc));
    formatDesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    formatDesc.index = 0;
    if (-1 == device.ioctl(VIDIOC_ENUM_FMT, &formatDesc))
    {
      /// {TODO} Track this device for later use for metadata
      continue;
    }

    // Add the camera
    int index = static_cast<int>(cameras.size());
    cameras.emplace_back(index, name, bus, path, driver, vid, pid);
  }
  return cameras;
}

FormatInfo CameraPlatform::OnSetFormat(const FormatInfo& info)  // info)
{
  v4l2_fmtdesc vfmtdesc;
  v4l2_frmsizeenum vsize;
  FormatInfo fmt_info = info;

  auto findFormat =
      [&](const v4l2_fmtdesc& fmtdesc, const v4l2_frmsizeenum& frmsize, const FormatInfo& checkFmt)
  {
    if (info.Matches(checkFmt))
    {
      fmt_info = checkFmt;
      vfmtdesc = fmtdesc;
      vsize    = frmsize;
      return false;
    }
    return true;
  };

  // Enumerate modes with the lambda above to find the matching format
  impl_->EnumerateModes(findFormat);

  // ok, now got the match requested...
  v4l2_format vfmt;
  memset(&vfmt, 0, sizeof(vfmt));
  vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  // Get the current format
  int result = impl_->device_->ioctl(VIDIOC_G_FMT, &vfmt);
  if (-1 == result)
  {
    ZBA_THROW("Unable to get format", Result::ZBA_UNSUPPORTED_FMT);
  }

  // Set to user selected format params
  v4l2_pix_format& pfmt = reinterpret_cast<v4l2_pix_format&>(vfmt.fmt);

  vfmt.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  pfmt.pixelformat = vfmtdesc.pixelformat;
  pfmt.width       = vsize.discrete.width;
  pfmt.height      = vsize.discrete.height;

  // Set the format
  result = impl_->device_->ioctl(VIDIOC_S_FMT, &vfmt);
  if (-1 == result)
  {
    ZBA_THROW("Unable to set format", Result::ZBA_UNSUPPORTED_FMT);
  }

  return fmt_info;
}

void CameraPlatform::Impl::Start()
{
  buffers_ = std::make_unique<BufferGroup>(device_, kNumBuffers);

  // Start streaming
  unsigned int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (-1 == device_->ioctl(VIDIOC_STREAMON, &type))
  {
    ZBA_THROW("Error starting streaming!", Result::ZBA_CAMERA_ERROR);
  }
  camera_thread_ = std::thread(&CameraPlatform::Impl::CaptureThread, this);
  started_       = true;
}

void CameraPlatform::Impl::Stop()
{
  // Stop streaming
  if (camera_thread_.joinable())
  {
    camera_thread_.join();
  }

  if (started_)
  {
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == device_->ioctl(VIDIOC_STREAMOFF, &type))
    {
      ZBA_THROW("Error stopping streaming!", Result::ZBA_CAMERA_ERROR);
    }
    // Unmap
    buffers_.reset();
    started_ = false;
  }
}

void CameraPlatform::Impl::CaptureThread()
{
  // Queue up buffers
  buffers_->QueueAll();

  size_t bufIdx = 0;
  while (!parent_.exiting_)
  {
    int result = device_->select(5.0f);

    if (-1 == result)
    {
      if ((EINTR == errno) || (EAGAIN == errno)) continue;
    }
    else if (0 == result)
    {
      ZBA_LOG("Frame timed out on {}", parent_.info_.name);
      continue;
    }

    // Get a buffer
    if (!buffers_->Get(bufIdx).Dequeue()) continue;
    auto format_if_set = parent_.GetFormat();
    if (format_if_set)
    {
      auto format = *format_if_set;
      /*
      if (parent_.decode_ == DecodeType::SYSTEM)
      {
        // If we've got V4L2 converting to BGRA for us {TODO}
        BGRAToBGRAFrame(buffers_[buffer.index].data, parent_.cur_frame_, src_stride);
      }
      else
      */

      /// {TODO} Don't have system decoding yet for Linux, soon....
      /// Also fix decisions so we're not doing compares like this.
      if ((parent_.decode_ == DecodeType::SYSTEM) || (parent_.decode_ == DecodeType::INTERNAL))
      {
        if (format.format == "GREY")
        {
          int src_stride = parent_.cur_frame_.width();
          GreyToFrame(reinterpret_cast<uint8_t*>(buffers_->Get(bufIdx).Data()), parent_.cur_frame_,
                      src_stride);
        }
        else if (format.format == "Z16 ")
        {
          int src_stride = parent_.cur_frame_.width() * 2;
          GreyToFrame(reinterpret_cast<uint8_t*>(buffers_->Get(bufIdx).Data()), parent_.cur_frame_,
                      src_stride);
        }
        else if (format.format == "YUYV")
        {
          int src_stride = (parent_.cur_frame_.width() * 2);
          YUY2ToBGRFrame(reinterpret_cast<uint8_t*>(buffers_->Get(bufIdx).Data()),
                         parent_.cur_frame_, src_stride);
        }
        else if (format.format == "NV12")
        {
          int src_stride = (parent_.cur_frame_.width());
          NV12ToBGRFrame(reinterpret_cast<uint8_t*>(buffers_->Get(bufIdx).Data()),
                         parent_.cur_frame_, src_stride);
        }
      }
      else
      {
        parent_.CopyRawBuffer(buffers_->Get(bufIdx).Data());
      }
    }
    parent_.OnFrameReceived(parent_.cur_frame_);

    // Requeue buffer
    buffers_->Get(bufIdx).Queue();

    // Go to next buffer
    bufIdx = (bufIdx + 1) % kNumBuffers;
  }
  ZBA_LOG("CaptureThread exiting...");
}

CameraPlatform::Impl::Impl(CameraPlatform* parent)
    : parent_(*parent),
      device_(std::make_shared<DeviceV4L2>(parent_.info_.path)),
      started_(false)
{
  if (device_->bad())
  {
    ZBA_THROW("Error opening device(): " + parent_.info_.path, Result::ZBA_CAMERA_OPEN_FAILED);
  }

  v4l2_capability caps;
  memset(&caps, 0, sizeof(caps));
  if (-1 == device_->ioctl(VIDIOC_QUERYCAP, &caps))
  {
    ZBA_THROW("Error querying device: " + parent_.info_.name, Result::ZBA_CAMERA_OPEN_FAILED);
  }

  if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
  {
    ZBA_THROW("Capture interface not supported: " + parent_.info_.name,
              Result::ZBA_CAMERA_OPEN_FAILED);
  }

  auto saveFormat = [this](const v4l2_fmtdesc&, const v4l2_frmsizeenum&, const FormatInfo& fmt_info)
  {
    if (parent_.IsFormatSupported(fmt_info.format))
    {
      parent_.info_.AddFormat(fmt_info);
    }
    parent_.AddAllModeEntry(fmt_info);
    return true;
  };
  EnumerateModes(saveFormat);
  EnumerateControls();
}

void CameraPlatform::Impl::EnumerateModes(ModeCallback cb)
{
  struct v4l2_fmtdesc formatDesc;
  v4l2_frmsizeenum frameSize;

  // For each format type (e.g. YUYV, MJPG)
  for (int format_idx = 0;; format_idx++)
  {
    memset(&formatDesc, 0, sizeof(formatDesc));
    formatDesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    formatDesc.index = format_idx;

    if (-1 == device_->ioctl(VIDIOC_ENUM_FMT, &formatDesc))
    {
      break;
    }

    // For each frame size
    for (int fsize_idx = 0;; fsize_idx++)
    {
      memset(&frameSize, 0, sizeof(frameSize));
      frameSize.index        = fsize_idx;
      frameSize.pixel_format = formatDesc.pixelformat;
      if (-1 == device_->ioctl(VIDIOC_ENUM_FRAMESIZES, &frameSize))
      {
        break;
      }
      std::string format_str(reinterpret_cast<char*>(&frameSize.pixel_format), 4);
      float fps = 0.0f;
      /// {TODO} investigate stepwise sizes. For now, ignore them.
      if (V4L2_FRMSIZE_TYPE_DISCRETE != frameSize.type)
      {
        continue;
      }
      int width  = frameSize.discrete.width;
      int height = frameSize.discrete.height;

      v4l2_frmivalenum frmival;
      /// For each FPS
      for (int fival_idx = 0;; fival_idx++)
      {
        memset(&frmival, 0, sizeof(frmival));
        frmival.pixel_format = frameSize.pixel_format;
        frmival.width        = width;
        frmival.height       = height;
        frmival.index        = fival_idx;
        if (-1 == device_->ioctl(VIDIOC_ENUM_FRAMEINTERVALS, &frmival))
        {
          break;
        }

        if ((frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE) && (frmival.discrete.denominator > 0))
        {
          // Round to 2 significant digits, e.g. 29.97
          fps = std::round(100.0f / (static_cast<float>(frmival.discrete.numerator) /
                                     static_cast<float>(frmival.discrete.denominator))) /
                100.0f;
        }

        FormatInfo fmt_info(width, height, fps, format_str);
        if (cb)
        {
          // call callback
          bool result = cb(formatDesc, frameSize, fmt_info);
          if (!result) return;
        }
      }
    }
  }
}

void CameraPlatform::Impl::OnParamChanged(Param* param, bool raw_set, bool /*auto_mode*/)
{
  auto name = param->name();
  ZBA_LOG("Exposure {:x} Auto {:x}", V4L2_CID_EXPOSURE, V4L2_CID_EXPOSURE_AUTO);
  // Find control id
  int ctrl = 0;
  {
    std::lock_guard<std::mutex> lock(paramControlMutex_);
    auto iter = paramControlMap_.find(name);
    if (iter != paramControlMap_.end())
    {
      ctrl = iter->second;
    }
    else
    {
      ZBA_ERR("Didn't find matching control!");
      return;
    }
  }
  v4l2_control control;
  memset(&control, 0, sizeof(control));
  control.id = ctrl;

  // Ranged controls
  auto paramRanged = dynamic_cast<ParamRanged<double, double>*>(param);
  if (paramRanged)
  {
    ZBA_LOG("Param {}:{:x}: changed ( RawSet: {} - Raw:{} Scaled:{})", name, ctrl, raw_set,
            paramRanged->get(), paramRanged->getScaled());

    if (!raw_set)
    {
      /// {TODO} Add auto_mode support here if possible later.
      control.value = paramRanged->get();

      if (0 != device_->ioctl(VIDIOC_S_CTRL, &control))
      {
        ZBA_ERRNO("Error setting ranged control value on {}:{:x} to {}!", name, ctrl,
                  paramRanged->get());
      }
    }

    return;
  }

  // Menu controls
  auto paramMenu = dynamic_cast<ParamMenu*>(param);
  if (paramMenu)
  {
    ZBA_LOG("Param {}:{:x}: changed ( index {} value {} desc {})", name, ctrl,
            paramMenu->getIndex(), paramMenu->get(), paramMenu->to_string());

    if (!raw_set)
    {
      control.value = paramMenu->get();

      if (0 != device_->ioctl(VIDIOC_S_CTRL, &control))
      {
        ZBA_ERRNO("Error setting menu control value on {}:{:x} to {}!", name, ctrl,
                  paramMenu->get());
      }
    }
    return;
  }

  ZBA_LOG("Param {} changed. RawSet: {}", name, raw_set);
}

}  // namespace zebral
#endif  // __linux__