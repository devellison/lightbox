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

  /// Enumerates ALL the controls.
  void EnumerateAllControls();

  /// Thread that reads data from the camera and calls callbacks
  void CaptureThread();

  /// Starts the camera
  void Start();

  /// Stops the camera
  void Stop();

  /// Called when a param changes
  void OnParamChanged(Param* param, bool raw_set, bool auto_mode);

  /// Adds a parameter, optionally overriding its name
  /// Returns true on success (if supported)
  /// \param queryctrl - query structure from VIDIOC_QUERY_CTRL
  /// \param override_name - name if we want to override to a standard for param
  /// \param auto_id - the id of the auto control, or 0 if none.
  /// \param controlled_name - if this is an auto param, then the name of the param it controls
  ///                  empty otherwise.
  bool AddParameter(v4l2_queryctrl& queryctrl, const std::string& override_name, int auto_id,
                    const std::string& controlled_name = "");

  /// Adds a parameter manually from an id
  /// Returns true on success (if supported)
  bool AddParameter(int id, const std::string& name, int auto_id);

  bool AddAutoParameter(int base_id, int auto_id, const std::string& name);
  /// Check the the control is in auto mode.
  /// \param param - parameter to check (the base param, not the auto id)
  /// \return true if it is, false otherwise
  bool GetAutoMode(ParamRanged<double, double>* param);

  /// Set the control's auto mode
  /// \param param - parameter to set (the base param, not the auto id)
  /// \param to_auto - if true, set in automatic mode
  void SetAutoMode(ParamRanged<double, double>* param, bool to_auto);

  static constexpr int kNumBuffers = 1;   ///< Number of buffers to allocate
  std::unique_ptr<BufferGroup> buffers_;  ///< Buffer group for buffers
  CameraPlatform& parent_;                ///< Parent camera object
  DeviceV4L2Ptr device_;                  ///< Camera device file descriptor
  bool started_;                          ///< True if started.
  std::thread camera_thread_;

  std::mutex paramControlMutex_;
  std::map<std::string, int> paramControlMap_;  ///< name -> ctrl id

  // So, some auto controls are bool, but exposure is
  // a menu with values 1 and 3... so we're going to go ahead
  // and put them in as params themselves so the weird ones can be handled
  // but try to duplicate the automatic functionality we have on Windows
  // for the ones we know.
  std::map<std::string, std::shared_ptr<Param>> paramAutoParams_;  ///< name->param

  std::map<int, std::string> controlParamMap_;  //
};

bool CameraPlatform::Impl::AddParameter(v4l2_queryctrl& queryctrl, const std::string& override_name,
                                        int auto_id, const std::string& controlled_name)
{
  {
    std::lock_guard<std::mutex> lock(paramControlMutex_);
    // Already added.  We enumerated the control manually, now we're scanning
    // for other controls - so skip this one.
    if (controlParamMap_.count(queryctrl.id)) return true;
  }
  bool autoMode      = false;
  bool autoSupported = false;

  std::string name = override_name;
  if (name.empty())
  {
    name = reinterpret_cast<const char*>(&queryctrl.name[0]);
  }

  if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
  {
    // skip if disabled.
    ZBA_LOG("{} control is disabled by driver.", name);
    return false;
  }

  // Get the current value of the item
  double value = 0;
  device_->get_video_ctrl(queryctrl.id, value);

  {  // Map the name to the control id
    {
      std::lock_guard<std::mutex> lock(paramControlMutex_);
      ZBA_LOG("Adding {}:{:x}", name, queryctrl.id);
      paramControlMap_.emplace(name, queryctrl.id);
      controlParamMap_.emplace(queryctrl.id, name);
    }

    if (auto_id)
    {
      // Add the auto parameter
      if (AddAutoParameter(queryctrl.id, auto_id, name))
      {
        autoSupported = true;
        // Check if we're in auto mode
        device_->get_video_ctrl(auto_id, autoMode);
        ZBA_LOG("Found auto control for {}. Currently {}", name, autoMode);
      }
    }
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

    auto param    = new ParamMenu(name, callbacks, value, queryctrl.default_value);
    auto paramPtr = std::shared_ptr<Param>(dynamic_cast<Param*>(param));

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
      parent_.parameters_.emplace(name, paramPtr);
    }

    if (!controlled_name.empty())
    {
      std::lock_guard<std::mutex> lock(paramControlMutex_);
      paramAutoParams_.emplace(controlled_name, paramPtr);
    }
  }
  else
  {
    // Not a menu - ranged.  For now, translate everything to doubles,
    // but be sure to round when converting to value to send back.
    auto param = new ParamRanged<double, double>(
        name, callbacks, value, queryctrl.default_value, queryctrl.minimum, queryctrl.maximum,
        queryctrl.step, autoMode, autoSupported, RawToScaledNormal, ScaledToRawNormal);

    auto paramPtr = std::shared_ptr<Param>(dynamic_cast<Param*>(param));

    {  // Add to Camera's parameter list
      std::lock_guard<std::mutex> lock(parent_.parameter_mutex_);
      parent_.parameters_.emplace(name, paramPtr);
    }

    if (!controlled_name.empty())
    {
      std::lock_guard<std::mutex> lock(paramControlMutex_);
      paramAutoParams_.emplace(controlled_name, paramPtr);
    }
  }
  return true;
}

bool CameraPlatform::Impl::AddParameter(int id, const std::string& name, int auto_id = 0)
{
  v4l2_queryctrl queryctrl;
  memset(&queryctrl, 0, sizeof(queryctrl));
  queryctrl.id = id;

  if (0 == device_->ioctl(VIDIOC_QUERYCTRL, &queryctrl))
  {
    AddParameter(queryctrl, name, auto_id);
    return true;
  }
  ZBA_LOG("QueryCtrl failed for {} {:x}/{:x}", name, id, auto_id);
  return false;
}

bool CameraPlatform::Impl::AddAutoParameter(int base_id, int auto_id, const std::string& name)
{
  v4l2_queryctrl queryctrl;
  memset(&queryctrl, 0, sizeof(queryctrl));
  queryctrl.id = auto_id;

  if (0 != device_->ioctl(VIDIOC_QUERYCTRL, &queryctrl))
  {
    ZBA_ERRNO("Error querying auto parameter {} {:x}/{:x}", name, base_id, auto_id);
    return false;
  }

  if (AddParameter(queryctrl, "", 0, name))
  {
    return true;
  }
  return false;
}

void CameraPlatform::Impl::EnumerateControls()
{
  // Add the same controls as Windows with auto on/off supported
  AddParameter(V4L2_CID_EXPOSURE_ABSOLUTE, "Exposure", V4L2_CID_EXPOSURE_AUTO);
  AddParameter(V4L2_CID_FOCUS_ABSOLUTE, "Focus", V4L2_CID_FOCUS_AUTO);
  AddParameter(V4L2_CID_BRIGHTNESS, "Brightness", V4L2_CID_AUTOBRIGHTNESS);
  AddParameter(V4L2_CID_WHITE_BALANCE_TEMPERATURE, "WhiteBalance", V4L2_CID_AUTO_WHITE_BALANCE);
  // No auto
  AddParameter(V4L2_CID_CONTRAST, "Contrast");
  AddParameter(V4L2_CID_PAN_ABSOLUTE, "Pan");
  AddParameter(V4L2_CID_TILT_ABSOLUTE, "Tilt");
  AddParameter(V4L2_CID_ZOOM_ABSOLUTE, "Zoom");

  // Now add support for anything else we can query.
  // The goal is to give identical experience on different platforms
  // on things we can, but provide access to everything else available.
  ZBA_LOG("Unspecified params");
  EnumerateAllControls();
}

void CameraPlatform::Impl::EnumerateAllControls()
{
  v4l2_queryctrl queryctrl;
  memset(&queryctrl, 0, sizeof(queryctrl));
  queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;

  while (0 == device_->ioctl(VIDIOC_QUERYCTRL, &queryctrl))
  {
    AddParameter(queryctrl, "", 0);
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
  if (-1 == device_->start_video_stream())
  {
    ZBA_THROW("Error starting streaming!", Result::ZBA_CAMERA_ERROR);
  }
  camera_thread_ = std::thread(&CameraPlatform::Impl::CaptureThread, this);
  started_       = true;
}

void CameraPlatform::Impl::Stop()
{
  // Stop thread
  if (camera_thread_.joinable())
  {
    camera_thread_.join();
  }

  // Stop streaming
  if (started_)
  {
    if (-1 == device_->stop_video_stream())
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
      /// None of my cameras seem to produce them.
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

bool CameraPlatform::Impl::GetAutoMode(ParamRanged<double, double>* param)
{
  std::shared_ptr<Param> autoParam;
  {
    std::lock_guard<std::mutex> lock(paramControlMutex_);
    auto iter = paramAutoParams_.find(param->name());
    if (iter == paramAutoParams_.end())
    {
      return false;
    }
    autoParam = iter->second;
  }
  if (!autoParam) return false;

  auto paramRanged = dynamic_cast<ParamRanged<double, double>*>(autoParam.get());
  if (paramRanged)
  {
    return (paramRanged->getScaled() > 0.5) ? true : false;
  }

  auto paramMenu = dynamic_cast<ParamMenu*>(autoParam.get());
  if (paramMenu)
  {
    return (paramMenu->getIndex()) ? true : false;
  }

  return false;
}

void CameraPlatform::Impl::SetAutoMode(ParamRanged<double, double>* param, bool to_auto)
{
  std::shared_ptr<Param> autoParam;
  {
    std::lock_guard<std::mutex> lock(paramControlMutex_);
    auto iter = paramAutoParams_.find(param->name());
    if (iter == paramAutoParams_.end())
    {
      return;
    }
    autoParam = iter->second;
  }
  if (!autoParam) return;

  auto paramRanged = dynamic_cast<ParamRanged<double, double>*>(autoParam.get());
  if (paramRanged)
  {
    paramRanged->setScaled(to_auto);
    return;
  }

  auto paramMenu = dynamic_cast<ParamMenu*>(autoParam.get());
  if (paramMenu)
  {
    paramMenu->setIndex(to_auto ? 1 : 0);
    return;
  }
}

void CameraPlatform::Impl::OnParamChanged(Param* param, bool raw_set, bool auto_mode)
{
  auto name = param->name();

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

  auto paramRanged = dynamic_cast<ParamRanged<double, double>*>(param);
  // Ranged controls
  if (paramRanged)
  {
    if (!raw_set)
    {
      bool in_auto = false;

      if (paramRanged->autoSupported())
      {
        // Check if it's in auto mode here (set in_auto)
        in_auto = GetAutoMode(paramRanged);
        if (auto_mode != in_auto)
        {
          if (auto_mode)
          {
            // Set control to default value if we're setting auto, in case driver
            // reports supported but not really.
            device_->set_video_ctrl(ctrl, paramRanged->default_value());
          }

          // Set auto mode
          SetAutoMode(paramRanged, auto_mode);
        }
      }
      else if (auto_mode)
      {
        // Maybe set to default here? Auto requested but not supported
        ZBA_LOG("Auto change requested but control doesn't support. Setting to default.");
        auto_mode = false;
        paramRanged->setAuto(false, false);
        paramRanged->set(paramRanged->default_value());
        // Set value to ranged param value.
        device_->set_video_ctrl(ctrl, paramRanged->get());
        return;
      }

      if (!auto_mode)
      {
        if (0 != device_->set_video_ctrl(ctrl, paramRanged->get()))
        {
          ZBA_ERRNO("Error setting ranged control value on {}:{:x} to {}!", name, ctrl,
                    paramRanged->get());
        }
      }
      else
      {
        double value = paramRanged->get();

        // retrieve value from control
        device_->get_video_ctrl(ctrl, value);

        if (std::abs(value - paramRanged->get()) > std::numeric_limits<double>::epsilon())
        {
          // If it's in auto, and we're trying to set it from the GUI,
          // just update it from the hardware.
          paramRanged->set(value);
        }
      }
    }

    return;
  }

  // Menu controls
  auto paramMenu = dynamic_cast<ParamMenu*>(param);
  if (paramMenu)
  {
    if (!raw_set)
    {
      if (0 != device_->set_video_ctrl(ctrl, paramMenu->get()))
      {
        ZBA_ERRNO("Error setting menu control value on {}:{:x} to {}!", name, ctrl,
                  paramMenu->get());
      }
    }
    return;
  }
}

}  // namespace zebral
#endif  // __linux__