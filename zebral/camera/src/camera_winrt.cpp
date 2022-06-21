/// \file camera_winrt.cpp
/// CameraPlatform implementation for Windows in C++/WinRt
#if _WIN32
#include "camera_platform.hpp"

#include <algorithm>
#include <regex>

#include "convert.hpp"
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

  void CheckNewControlsSupported();

  /// Frame callback from the frame reader.
  void OnFrame(const Windows::Media::Capture::Frames::MediaFrameReader& reader,
               const Windows::Media::Capture::Frames::MediaFrameArrivedEventArgs&);

  /// Takes a device path string, returns vid/pid if found.
  /// \param busPath - Windows ID string (bus path)
  /// \param vid - receives USB vendorId on success
  /// \param pid - receives USB pid on success
  /// \return true if vid/pid found.
  static bool VidPidFromBusPath(const std::string& busPath, uint16_t& vid, uint16_t& pid);

  CameraPlatform& parent_;                       ///< CameraPlatform that owns this Impl
  MediaCapture mc_;                              ///< MediaCapture base object
  MediaCaptureInitializationSettings settings_;  ///< Settings for mc_
  IMapView<hstring, MediaFrameSource> sources_;  ///< FrameSources
  MediaFrameSource device_;                      ///< Chosen frame source after SetFormat
  MediaFrameReader reader_;                      ///< Reader once we've picked a source
  event_token reader_token_;                     ///< Callback token
  bool started_;                                 ///< True if started.
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
  if (impl_->reader_)
  {
    impl_->reader_token_ = impl_->reader_.FrameArrived({impl_.get(), &Impl::OnFrame});
    impl_->reader_.StartAsync().get();
    impl_->started_ = true;
  }
}

void CameraPlatform::OnStop()
{
  if (impl_->started_ && impl_->reader_)
  {
    impl_->reader_.FrameArrived(impl_->reader_token_);
    impl_->reader_.StopAsync().get();
    impl_->started_ = false;
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

std::string FilterSubtype(const winrt::hstring& format)
{
  std::string formatStr = winrt::to_string(format);
  while (formatStr.length() < 4)
  {
    formatStr.append(" ");
  }
  return formatStr;
}

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
              .CreateFrameReaderAsync(mediaFrameSource,
                                      (decode_ ? MediaEncodingSubtypes::Bgra8() : fourCC))
              .get();

      return formatInfo;
    }
  }
  ZBA_THROW("Could find requested format.", Result::ZBA_CAMERA_ERROR);
}

FormatInfo MediaFrameFormatToFormat(const MediaFrameFormat& curFormat)
{
  auto w         = curFormat.VideoFormat().Width();
  auto h         = curFormat.VideoFormat().Height();
  auto subType   = curFormat.Subtype();
  auto frameRate = curFormat.FrameRate();
  float fps      = std::round(100.0f * static_cast<float>(frameRate.Numerator()) /
                              static_cast<float>(frameRate.Denominator())) /
              100.0f;
  return FormatInfo(w, h, fps, FilterSubtype(subType));
}

CameraPlatform::Impl::Impl(CameraPlatform* parent)
    : parent_(*parent),
      reader_(nullptr),
      started_(false),
      device_(nullptr)
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
  //
  // I'm not sure why - this is something to investigate. But it causes a TryLookup() to fail.
  //
  // On the other hand.... we initialized the mediacapture object with just our device,
  // will it have other devices? It appears not, so just go through all the sources.
  // Some may be IR/DEPTH/etc. Need to test those.
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
}

void CameraPlatform::Impl::CheckNewControlsSupported()
{
  /// Test function...
  /// {TODO} Exposure controls and similar off VideoDeviceController() don't work.
  /// What it looks like is that they're unimplemented, so we'll have to dig deeper.
  /// Setting/Getting the KS properties directly appears to be how software that does this
  /// gets it done, so we'll go there.
  /// mc_.VideoDeviceController().GetDeviceProperty()
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
      if (parent_.decode_)
      {
        /// {TODO} Hardcoded bgra8, and doesn't account for padded stride or encodings.
        /// Right now if we switch to undecoded, we'll still have the original-sized
        /// buffer and it will display completely wrong.
        uint8_t* dataPtr = nullptr;
        uint32_t dataLen = 0;
        auto interop     = ref.as<IMemoryBufferByteAccess>();
        check_hresult(interop->GetBuffer(&dataPtr, &dataLen));

        BGRAToBGRFrame(dataPtr, parent_.cur_frame_, src_stride);
      }
      else
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

}  // namespace zebral
#endif  // _WIN32
