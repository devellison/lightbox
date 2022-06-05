#if _WIN32
#include "camera_winrt.hpp"

#include <regex>

#include "errors.hpp"
#include "log.hpp"
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

// Seems like some of the control features in the API aren't there yet, even on Windows 11?
// If this is on, calls and checks to see if they're enabled and logs.
#ifdef NDEBUG
#define CHECK_EXPOSURE_FEATURES 0
#else
#define CHECK_EXPOSURE_FEATURES 1
#endif

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
/// Fun implementation details.
class CameraWinRt::Impl
{
 public:
  Impl(CameraWinRt* parent)
      : parent_(*parent),
        reader_(nullptr),
        exposure_(nullptr),
        started_(false),
        device_(nullptr)
  {
  }

  /// {TODO} So.... these return false for my cameras on my system.
  /// SDK 10.0.19041.0, Windows 10 Home 10.0.19041.
  /// Some things seem to say that it's just not ready yet on Windows 11
  /// It seems like you need to go through older APIs currently, which is painful.
  void CheckNewControlsSupported()
  {
    ZBA_LOG("Checking VideoDeviceController controls...");
    ZBA_LOG(" ExposureControl %d", mc_.VideoDeviceController().ExposureControl().Supported());
    ZBA_LOG(" ExposureCompensationControl %d",
            mc_.VideoDeviceController().ExposureCompensationControl().Supported());
    ZBA_LOG(" WhiteBalanceControl %d",
            mc_.VideoDeviceController().WhiteBalanceControl().Supported());
    ZBA_LOG(" ExposurePriorityVideoControl %d",
            mc_.VideoDeviceController().ExposurePriorityVideoControl().Supported());
    ZBA_LOG(" FocusControl %d", mc_.VideoDeviceController().FocusControl().Supported());
    ZBA_LOG(" FlashControl %d", mc_.VideoDeviceController().FlashControl().Supported());
    ZBA_LOG(" ISO Speed Control %d", mc_.VideoDeviceController().IsoSpeedControl().Supported());
    ZBA_LOG(" HDRVideoControl %d", mc_.VideoDeviceController().HdrVideoControl().Supported());
    // Because one of them had to be different.
    ZBA_LOG(" IRTorchControl %d", mc_.VideoDeviceController().InfraredTorchControl().IsSupported());

    // Checking device
    ZBA_LOG(" Checking device....");
    ZBA_LOG(" ExposureCompensationControl %d",
            device_.Controller().VideoDeviceController().ExposureCompensationControl().Supported());

    ZBA_LOG(" WhiteBalanceControl %d",
            device_.Controller().VideoDeviceController().WhiteBalanceControl().Supported());

    ZBA_LOG(
        " ExposurePriorityVideoControl %d",
        device_.Controller().VideoDeviceController().ExposurePriorityVideoControl().Supported());
    ZBA_LOG(" FocusControl %d",
            device_.Controller().VideoDeviceController().FocusControl().Supported());
    ZBA_LOG(" FlashControl %d",
            device_.Controller().VideoDeviceController().FlashControl().Supported());
    ZBA_LOG(" ISO Speed Control %d",
            device_.Controller().VideoDeviceController().IsoSpeedControl().Supported());
    ZBA_LOG(" HDRVideoControl %d",
            device_.Controller().VideoDeviceController().HdrVideoControl().Supported());
    // Because one of them had to be different.
    ZBA_LOG(" IRTorchControl %d",
            device_.Controller().VideoDeviceController().InfraredTorchControl().IsSupported());

    // mc_.VideoDeviceController().BacklightCompensation().TrySetAuto(true);
    // mc_.VideoDeviceController().Brightness().TrySetAuto(true);
    // mc_.VideoDeviceController().Contrast().TrySetAuto(true);
    // mc_.VideoDeviceController().ExposureControl().SetAutoAsync(true).get();
  }

  /// Frame callback from the frame reader.
  void OnFrame(const Windows::Media::Capture::Frames::MediaFrameReader& reader,
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

        /// {TODO} Hardcoded bgra8, and doesn't account for padded stride.
        int width             = plane_desc.Width;
        int height            = plane_desc.Height;
        int channels          = plane_desc.Stride / width;
        int bytes_per_channel = 1;
        bool is_signed        = false;
        bool is_floating      = false;
        CameraFrame cameraFrame(width, height, channels, bytes_per_channel, is_signed, is_floating);

        IMemoryBufferReference ref = bmpBuffer.CreateReference();
        {
          auto interop = ref.as<IMemoryBufferByteAccess>();

          uint8_t* dataPtr = nullptr;
          uint32_t dataLen = 0;
          check_hresult(interop->GetBuffer(&dataPtr, &dataLen));
          memcpy(cameraFrame.data(), dataPtr, dataLen);
        }
        ref.Close();
        bmpBuffer.Close();
        parent_.OnFrameReceived(cameraFrame);
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

  CameraWinRt& parent_;                          ///< CameraWinRT that owns this Impl
  MediaCapture mc_;                              ///< MediaCapture base object
  MediaCaptureInitializationSettings settings_;  ///< Settings for mc_
  IMapView<hstring, MediaFrameSource> sources_;  ///< FrameSources
  MediaFrameReader reader_;                      ///< Reader once we've picked a source
  ExposureControl exposure_;                     ///< Control for exposure of camera
  event_token reader_token_;                     ///< Callback token
  bool started_;                                 ///< True if started.
  MediaFrameSource device_;
};

FormatInfo MediaFrameFormatToFormat(const MediaFrameFormat& curFormat)
{
  auto w         = curFormat.VideoFormat().Width();
  auto h         = curFormat.VideoFormat().Height();
  auto subType   = curFormat.Subtype();
  auto frameRate = curFormat.FrameRate();
  float fps =
      static_cast<float>(frameRate.Numerator()) / static_cast<float>(frameRate.Denominator());
  return FormatInfo(w, h, fps, 0, 0, 0, winrt::to_string(subType));
}

// CameraWin main class
// COM interfaces and most of the code
// is kept in implementation class below
CameraWinRt::CameraWinRt(const CameraInfo& info) : Camera(info)
{
  impl_            = std::make_unique<Impl>(this);
  auto devices     = Windows::Media::Capture::Frames::MediaFrameSourceGroup::FindAllAsync().get();
  bool initialized = false;

  for (auto curDevice : devices)
  {
    std::string deviceId = winrt::to_string(curDevice.Id());
    if (deviceId == info_.path)
    {
      impl_->settings_.SourceGroup(curDevice);
      impl_->settings_.SharingMode(MediaCaptureSharingMode::ExclusiveControl);
      impl_->settings_.MemoryPreference(MediaCaptureMemoryPreference::Cpu);
      impl_->settings_.StreamingCaptureMode(StreamingCaptureMode::Video);

      /// MediaCapture can throw - catch and rethrow as our own error type.
      try
      {
        impl_->mc_.InitializeAsync(impl_->settings_).get();
        initialized = true;
        break;
      }
      catch (winrt::hresult_error const& ex)
      {
        ZBA_ERR(winrt::to_string(ex.message()).c_str());
        ZBA_THROW("Unable to initialize capture for device: " + info_.name,
                  Result::ZBA_CAMERA_OPEN_FAILED);
      }
    }
  }

  if (!initialized)
  {
    ZBA_THROW("Didn't find requested capture device: " + info_.name,
              Result::ZBA_CAMERA_OPEN_FAILED);
  }

  /// Now enumerate modes...
  auto frameSources = impl_->mc_.FrameSources();

  // {TODO} Okay, this is weird. The frameSources map has the sources, with Source#0@ prepending
  // to their IDs. Also the case on the ending /global is different. Go figure.
  //
  // I'm not sure why - this is something to investigate. But it causes a TryLookup() to fail.
  //
  // On the other hand.... we initialized the mediacapture object with just our device,
  // will it have other devices? It appears not, so just go through all the sources.
  // Some may be IR/DEPTH/etc. Need to test those.
  for (const auto& curSource : frameSources)
  {
    impl_->device_ = curSource.Value();
    auto formats   = impl_->device_.SupportedFormats();
    for (const auto& curFormat : formats)
    {
      // Skip non-Video formats.
      auto major = curFormat.MajorType();
      if (major != L"Video") continue;
      info_.formats.emplace_back(MediaFrameFormatToFormat(curFormat));
    }
  }
}

CameraWinRt::~CameraWinRt() {}

void CameraWinRt::OnStart()
{
  if (impl_->reader_)
  {
    impl_->reader_token_ = impl_->reader_.FrameArrived({impl_.get(), &Impl::OnFrame});
    impl_->reader_.StartAsync().get();

#if CHECK_EXPOSURE_FEATURES
    impl_->CheckNewControlsSupported();
#endif

    impl_->started_ = true;
  }
}

void CameraWinRt::OnStop()
{
  if (impl_->started_ && impl_->reader_)
  {
    impl_->reader_.FrameArrived(impl_->reader_token_);
    impl_->reader_.StopAsync().get();
    impl_->started_ = false;
  }
}

bool CameraWinRt::VidPidFromPath(const std::string& devPath, uint16_t& vid, uint16_t& pid)
{
  if (devPath.empty()) return false;

  // MediaFoundations returns this in lowercase, C++/WinRT returns it in uppercase.
  static const std::regex vidPidRe("usb#vid_([0-9a-z]{4})&pid_([0-9a-z]{4})",
                                   std::regex_constants::ECMAScript | std::regex_constants::icase);

  std::smatch matches;
  if (std::regex_search(devPath, matches, vidPidRe))
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

std::vector<CameraInfo> CameraWinRt::Enumerate()
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
        // We really should support depth and infrared as well at the least.
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
    std::string deviceId   = winrt::to_string(curDevice.Id());

    /// If it's not a USB device, VidPid returns false and vid/pid remain 0.
    uint16_t vid = 0;
    uint16_t pid = 0;
    VidPidFromPath(deviceId, vid, pid);

    int index = static_cast<int>(cameras.size());
    cameras.emplace_back(index, 0, deviceName, deviceId, nullptr, vid, pid);
  }
  return cameras;
}

void CameraWinRt::OnSetFormat(const FormatInfo& info)
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

      auto formatInfo = MediaFrameFormatToFormat(curFormat);

      // Find something that matches. Unset entries are wildcards.
      if ((formatInfo.width != info.width) && (info.width != 0)) continue;
      if ((formatInfo.height != info.height) && (info.height != 0)) continue;
      if ((formatInfo.channels != info.channels) && (info.channels != 0)) continue;
      if ((formatInfo.format != info.format) && (!info.format.empty())) continue;

      // Set the format
      mediaFrameSource.SetFormatAsync(curFormat).get();
      impl_->reader_ =
          impl_->mc_.CreateFrameReaderAsync(mediaFrameSource, MediaEncodingSubtypes::Bgra8()).get();
      return;
    }
  }
  ZBA_THROW("Could find requested format.", Result::ZBA_CAMERA_ERROR);
}

}  // namespace zebral
#endif  // _WIN32