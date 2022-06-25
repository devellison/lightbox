/// \file camera_winrt.cpp
/// CameraPlatform implementation for Windows in C++/WinRt
#if _WIN32
#include "camera_platform.hpp"

#include <algorithm>
#include <memory>
#include <regex>

#include "convert.hpp"
#include "errors.hpp"
#include "log.hpp"
#include "param.hpp"
#include "platform.hpp"

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Capture.Frames.h>
#include <winrt/Windows.Media.Capture.h>
#include <winrt/Windows.Media.Core.h>
#include <winrt/Windows.Media.Devices.h>
#include <winrt/Windows.Media.MediaProperties.h>
#include <winrt/Windows.Media.h>

#include <winrt/base.h>

// Required to gain access to software bitmap pixels directly.
struct __declspec(uuid("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d")) __declspec(novtable)
    IMemoryBufferByteAccess : ::IUnknown
{
  virtual HRESULT __stdcall GetBuffer(uint8_t** value, uint32_t* capacity) = 0;
};

using namespace winrt;

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Media;
using namespace Windows::Media::Capture;
using namespace Windows::Media::Capture::Frames;
using namespace Windows::Media::Core;
using namespace Windows::Media::Devices;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Storage::Streams;

namespace zebral
{
/// Utility function to convert Windows format to ours
FormatInfo MediaFrameFormatToFormat(const MediaFrameFormat& curFormat);

/// Fun implementation details.
class CameraPlatform::Impl
{
 public:
  Impl(CameraPlatform* parent);

  /// Frame callback from the frame reader.
  void OnFrame(const Windows::Media::Capture::Frames::MediaFrameReader& reader,
               const Windows::Media::Capture::Frames::MediaFrameArrivedEventArgs&);

  /// On capture start
  void Start();

  /// On capture stop
  void Stop();

  /// Takes a device path string, returns vid/pid if found.
  /// \param busPath - Windows ID string (bus path)
  /// \param vid - receives USB vendorId on success
  /// \param pid - receives USB pid on success
  /// \return true if vid/pid found.
  static bool VidPidFromBusPath(const std::string& busPath, uint16_t& vid, uint16_t& pid);

  void OnParamChanged(Param* param, bool raw_set, bool auto_mode);
  void AddParameter(const std::string& name, MediaDeviceControl ctrl, bool supportsAuto);

  CameraPlatform& parent_;                       ///< CameraPlatform that owns this Impl
  MediaCapture mc_;                              ///< MediaCapture base object
  MediaCaptureInitializationSettings settings_;  ///< Settings for mc_
  IMapView<hstring, MediaFrameSource> sources_;  ///< FrameSources
  MediaFrameSource device_;                      ///< Chosen frame source after SetFormat
  MediaFrameReader reader_;                      ///< Reader once we've picked a source
  event_token reader_token_;                     ///< Callback token
  bool started_;                                 ///< True if started.

  VideoDeviceController videoDevCtrl_;  ///< VideoDeviceController for the device
  std::mutex paramControlMutex_;        ///< Lock for map
  std::map<std::string, MediaDeviceControl> paramControlMap_;  ///< maps param names to controls
};

// CameraWin main class
// COM interfaces and most of the code
// is kept in implementation class below
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

void CameraPlatform::Impl::Start()
{
  if (reader_)
  {
    reader_token_ = reader_.FrameArrived({this, &Impl::OnFrame});
    reader_.StartAsync().get();
    started_ = true;
  }
}

void CameraPlatform::Impl::Stop()
{
  if (started_ && reader_)
  {
    reader_.FrameArrived(reader_token_);
    reader_.StopAsync().get();
    started_ = false;
  }
}

bool CameraPlatform::Impl::VidPidFromBusPath(const std::string& busPath, uint16_t& vid,
                                             uint16_t& pid)
{
  if (busPath.empty()) return false;

  // MediaFoundations returns this in lowercase, C++/WinRT returns it in uppercase.
  static const std::regex vidPidRe("usb#vid_([0-9a-z]{4})&pid_([0-9a-z]{4})",
                                   std::regex_constants::ECMAScript | std::regex_constants::icase);

  std::smatch matches;
  if (std::regex_search(busPath, matches, vidPidRe))
  {
    if (matches.size() == 3)
    {
      vid = static_cast<uint16_t>(std::stoul(std::string("0x") + matches[1].str(), nullptr, 16));
      pid = static_cast<uint16_t>(std::stoul(std::string("0x") + matches[2].str(), nullptr, 16));
      return true;
    }
  }
  return false;
}

std::vector<CameraInfo> CameraPlatform::Enumerate()
{
  std::vector<CameraInfo> cameras;
  auto devices = Windows::Media::Capture::Frames::MediaFrameSourceGroup::FindAllAsync().get();

  for (const auto& curDevice : devices)
  {
    auto sources        = curDevice.SourceInfos();
    bool foundSupported = false;
    for (const auto& sourceInfo : sources)
    {
      if (sourceInfo.MediaStreamType() == MediaStreamType::VideoRecord)
      {
        /// {TODO} We really should support depth and infrared as well at the least.
        if (sourceInfo.SourceKind() == MediaFrameSourceKind::Color)
        {
          foundSupported = true;
        }
      }
    }
    if (!foundSupported)
    {
      continue;
    }

    std::string deviceName = winrt::to_string(curDevice.DisplayName());
    std::string bus_path   = winrt::to_string(curDevice.Id());

    // These two are empty currently.
    std::string path;
    std::string driver;

    // If it's not a USB device, VidPid returns false and vid/pid remain 0.
    uint16_t vid = 0;
    uint16_t pid = 0;
    Impl::VidPidFromBusPath(bus_path, vid, pid);

    int index = static_cast<int>(cameras.size());
    cameras.emplace_back(index, deviceName, bus_path, path, driver, vid, pid);
  }
  return cameras;
}

/// Convert from windows hstring to std::string for fourCC formats.
///
/// Note that Windows fourCC strings are sometimes terminated early,
/// e.g. "L8" instead of "L8  ", whereas V4L2 strings are space
/// padded.
///
/// Also note that sometimes they just stick a full GUID string or
/// something else random in, so we need to back away from using
/// the int32 <-> fourCC conversion.
std::string FilterSubtype(const winrt::hstring& format)
{
  std::string formatStr = winrt::to_string(format);
  while (formatStr.length() < 4)
  {
    formatStr.append(" ");
  }
  return formatStr;
}

/// Convert from std::string to windows hstring for fourCC
/// formats.  This also removes any padding.
winrt::hstring FilterFormat(const std::string& format)
{
  std::string formatStr = format;

  auto pos = formatStr.find_first_of(' ');
  if (pos != std::string::npos)
  {
    formatStr = formatStr.substr(0, pos);
  }
  return winrt::to_hstring(formatStr);
}

FormatInfo CameraPlatform::OnSetFormat(const FormatInfo& info)
{
  impl_->sources_ = impl_->mc_.FrameSources();

  for (const auto& curSource : impl_->sources_)
  {
    auto mediaFrameSource = curSource.Value();
    auto formats          = mediaFrameSource.SupportedFormats();
    for (const auto& curFormat : formats)
    {
      // Skip non-Video formats.
      auto major = curFormat.MajorType();
      if (major != L"Video") continue;
      if (!IsFormatSupported(FilterSubtype(curFormat.Subtype()))) continue;

      auto formatInfo = MediaFrameFormatToFormat(curFormat);

      // Find something that matches. Unset entries are wildcards.
      if (!formatInfo.Matches(info)) continue;

      // Set the format
      mediaFrameSource.SetFormatAsync(curFormat).get();
      winrt::hstring fourCC = FilterFormat(formatInfo.format);

      /// {TODO} This switches decoding on and off, but should also be able to switch
      /// to different greyscale image types and similar.
      /// Much work to be done on decoding still.
      impl_->reader_ =
          impl_->mc_
              .CreateFrameReaderAsync(
                  mediaFrameSource,
                  ((decode_ == DecodeType::SYSTEM) ? MediaEncodingSubtypes::Bgra8() : fourCC))
              .get();

      return formatInfo;
    }
  }
  ZBA_THROW("Could find requested format.", Result::ZBA_CAMERA_ERROR);
}

FormatInfo MediaFrameFormatToFormat(const MediaFrameFormat& curFormat)
{
  auto frameRate = curFormat.FrameRate();
  float fps      = std::round(100.0f * static_cast<float>(frameRate.Numerator()) /
                              static_cast<float>(frameRate.Denominator())) /
              100.0f;

  return FormatInfo(curFormat.VideoFormat().Width(), curFormat.VideoFormat().Height(), fps,
                    FilterSubtype(curFormat.Subtype()));
}

CameraPlatform::Impl::Impl(CameraPlatform* parent)
    : parent_(*parent),
      reader_(nullptr),
      started_(false),
      device_(nullptr),
      videoDevCtrl_(nullptr)
{
  auto devices     = Windows::Media::Capture::Frames::MediaFrameSourceGroup::FindAllAsync().get();
  bool initialized = false;

  for (auto curDevice : devices)
  {
    std::string deviceId = winrt::to_string(curDevice.Id());
    if (deviceId == parent_.info_.bus)
    {
      settings_.SourceGroup(curDevice);
      settings_.SharingMode(MediaCaptureSharingMode::ExclusiveControl);
      settings_.MemoryPreference(MediaCaptureMemoryPreference::Cpu);
      settings_.StreamingCaptureMode(StreamingCaptureMode::Video);

      /// MediaCapture can throw - catch and rethrow as our own error type.
      try
      {
        mc_.InitializeAsync(settings_).get();
        initialized = true;
        break;
      }
      catch (winrt::hresult_error const& ex)
      {
        ZBA_ERR(winrt::to_string(ex.message()));
        ZBA_THROW("Unable to initialize capture for device: " + parent_.info_.name,
                  Result::ZBA_CAMERA_OPEN_FAILED);
      }
    }
  }

  if (!initialized)
  {
    ZBA_THROW("Didn't find requested capture device: " + parent_.info_.name,
              Result::ZBA_CAMERA_OPEN_FAILED);
  }

  /// Now enumerate modes...
  auto frameSources = mc_.FrameSources();

  // {TODO} Okay, this is weird. The frameSources map has the sources, with Source#0@ prepending
  // to their IDs. Also the case on the ending /global is different. Go figure.
  for (const auto& curSource : frameSources)
  {
    device_      = curSource.Value();
    auto formats = device_.SupportedFormats();
    for (const auto& curFormat : formats)
    {
      // Skip non-Video formats.
      auto major = curFormat.MajorType();
      if (major != L"Video") continue;
      auto format = MediaFrameFormatToFormat(curFormat);
      if (parent_.IsFormatSupported(FilterSubtype(curFormat.Subtype())))
      {
        parent_.info_.AddFormat(format);
      }
      parent_.AddAllModeEntry(format);
    }
  }

  // Get the device controller for parameters
  videoDevCtrl_ = mc_.VideoDeviceController();

  // These support auto
  AddParameter("Exposure", videoDevCtrl_.Exposure(), true);
  AddParameter("Focus", videoDevCtrl_.Focus(), true);
  AddParameter("Brightness", videoDevCtrl_.Brightness(), true);
  AddParameter("WhiteBalance", videoDevCtrl_.WhiteBalance(), true);

  // These don't, setting auto sets to default
  AddParameter("Contrast", videoDevCtrl_.Contrast(), false);
  AddParameter("Pan", videoDevCtrl_.Pan(), false);
  AddParameter("Tilt", videoDevCtrl_.Tilt(), false);
  AddParameter("Zoom", videoDevCtrl_.Zoom(), false);
}

void CameraPlatform::Impl::AddParameter(const std::string& name, MediaDeviceControl ctrl,
                                        bool autoSupport)
{
  if (ctrl)
  {
    auto caps = ctrl.Capabilities();
    if (caps.Supported())
    {
      {
        std::lock_guard<std::mutex> lock(paramControlMutex_);
        paramControlMap_.emplace(name, ctrl);
      }
      bool automode = false;
      ctrl.TryGetAuto(automode);
      double value = 0.0;
      ctrl.TryGetValue(value);
      ParamSubscribers callbacks{
          {name, std::bind(&CameraPlatform::Impl::OnParamChanged, this, std::placeholders::_1,
                           std::placeholders::_2, std::placeholders::_3)}};
      auto param = new ParamRanged<double, double>(
          name, callbacks, value, caps.Default(), caps.Min(), caps.Max(), caps.Step(), automode,
          autoSupport, RawToScaledNormal, ScaledToRawNormal);

      std::lock_guard<std::mutex> lock(parent_.parameter_mutex_);
      parent_.parameters_.emplace(name, std::shared_ptr<Param>(dynamic_cast<Param*>(param)));
    }
  }
}

void CameraPlatform::Impl::OnFrame(
    const Windows::Media::Capture::Frames::MediaFrameReader& reader,
    const Windows::Media::Capture::Frames::MediaFrameArrivedEventArgs&)
{
  if (auto frame = reader.TryAcquireLatestFrame())
  {
    auto bitmap        = frame.VideoMediaFrame().SoftwareBitmap();
    auto format_if_set = parent_.GetFormat();
    if (format_if_set)
    {
      auto format            = *format_if_set;
      BitmapBuffer bmpBuffer = bitmap.LockBuffer(BitmapBufferAccessMode::Read);

      auto plane_desc = bmpBuffer.GetPlaneDescription(0);
      int src_stride  = plane_desc.Stride;

      IMemoryBufferReference ref = bmpBuffer.CreateReference();
      // The system is doing the decoding for us.
      if (parent_.decode_ == DecodeType::SYSTEM)
      {
        /// {TODO} Hardcoded bgra8, need to add options for grey/depth
        uint8_t* dataPtr = nullptr;
        uint32_t dataLen = 0;
        auto interop     = ref.as<IMemoryBufferByteAccess>();
        check_hresult(interop->GetBuffer(&dataPtr, &dataLen));

        BGRAToBGRFrame(dataPtr, parent_.cur_frame_, src_stride);
      }
      else if (parent_.decode_ == DecodeType::INTERNAL)
      {
        // For now we'll try decoding it...
        // We'll also want a raw option.
        uint8_t* dataPtr = nullptr;
        uint32_t dataLen = 0;
        auto interop     = ref.as<IMemoryBufferByteAccess>();
        check_hresult(interop->GetBuffer(&dataPtr, &dataLen));

        auto srcPtr = dataPtr;
        // my system stats
        // (800, 448) is about 0.026s in debug mode, 0.0018s in release mode (no parallel, pure cpp)
        if (format.format == "YUY2")
        {
          // ZBA_TIMER(timer, "YUY2ToBGRFrame");
          YUY2ToBGRFrame(srcPtr, parent_.cur_frame_, src_stride);
        }
        else if (format.format == "NV12")
        {
          // ZBA_TIMER(timer, "NV12ToBGRFrame");
          NV12ToBGRFrame(srcPtr, parent_.cur_frame_, src_stride);
        }
        else if (format.format == "D16 ")
        {
          GreyToFrame(srcPtr, parent_.cur_frame_, src_stride);
        }
        else if (format.format == "L8  ")
        {
          GreyToFrame(srcPtr, parent_.cur_frame_, src_stride);
        }
        else
        {
          ZBA_ERR("Don't currently have a converter for {}", format.format);
        }
      }
      else if (parent_.decode_ == DecodeType::NONE)
      {
        uint8_t* dataPtr = nullptr;
        uint32_t dataLen = 0;
        auto interop     = ref.as<IMemoryBufferByteAccess>();
        check_hresult(interop->GetBuffer(&dataPtr, &dataLen));
        parent_.CopyRawBuffer(dataPtr, src_stride);
      }

      ref.Close();
      bmpBuffer.Close();
      parent_.OnFrameReceived(parent_.cur_frame_);
    }
    else
    {
      ZBA_LOG("Format not set.");
    }
  }
  else
  {
    ZBA_LOG("Failed aquire latest.");
  }
}

void CameraPlatform::Impl::OnParamChanged(Param* param, bool raw_set, bool auto_mode)
{
  auto name        = param->name();
  auto paramRanged = dynamic_cast<ParamRanged<double, double>*>(param);

  // Ranged controls
  if (paramRanged)
  {
    MediaDeviceControl ctrl(nullptr);
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

    ZBA_LOG("Param {} changed ( RawSet: {} - Raw:{} Scaled:{})", name, raw_set, paramRanged->get(),
            paramRanged->getScaled());

    // If Raw isn't set, is from host - set the hardware
    if ((!raw_set) && ctrl)
    {
      bool in_auto = false;

      if (paramRanged->autoSupported())
      {
        ctrl.TryGetAuto(in_auto);
        if (auto_mode != in_auto)
        {
          if (auto_mode)
          {
            // Set to default when switching to auto mode, in case
            // auto mode is unsupported by hardware.
            ctrl.TrySetValue(paramRanged->default_value());
          }
          ctrl.TrySetAuto(auto_mode);
        }
      }
      else if (auto_mode)
      {
        // Maybe set to default here? Auto requested but not supported
        ZBA_LOG("Auto change requested but control doesn't support. Setting to default.");
        auto_mode = false;
        paramRanged->setAuto(false, false);
        paramRanged->set(paramRanged->default_value());
        ctrl.TrySetValue(paramRanged->get());
        return;
      }

      if (!auto_mode)
      {
        ctrl.TrySetValue(paramRanged->get());
      }
      else
      {
        double value = paramRanged->get();
        ctrl.TryGetValue(value);
        if (std::abs(value - paramRanged->get()) > std::numeric_limits<double>::epsilon())
        {
          // If it's in auto, and we're trying to set it from the GUI,
          // just update it from the hardware.
          paramRanged->set(value);
          ZBA_LOG("Auto mode. Queried: Value: {} Scaled: {}", paramRanged->get(),
                  paramRanged->getScaled());
        }
      }
    }
  }
  else
  {
    ZBA_LOG("Param {} changed. RawSet: {}", name, raw_set);
  }
}

}  // namespace zebral
#endif  // _WIN32
