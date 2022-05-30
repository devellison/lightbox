#include "camera_manager.hpp"
#include <functional>
#include <regex>
#include "camera_info.hpp"
#include "camera_win.hpp"
#include "errors.hpp"
#include "platform.hpp"

#ifdef _WIN32
#define kPREF_API cv::CAP_DSHOW
#include <Mfidl.h>
#include <dshow.h>
#include <mfapi.h>
#include <windows.h>
#elif defined(__linux__)
#define kPREF_API cv::CAP_V4L2
#else
#define kPREF_API cv::CAP_ANY
#endif

namespace zebral
{
// Remove when we remove OpenCV
// Set kPREF_API to the preferred api for the platform

CameraManager::CameraManager() {}

#if 0
// This is temporary - OpenCV sucks for dealing directly with cameras.
// Replace with platform specific code.
std::vector<CameraInfo> EnumerateOpenCV()
{
  std::vector<CameraInfo> cameras;
  // OpenCV doesn't give a count of devices.
  // Or device names, or really anything useful at all.
  const int kMaxCamSearch = 10;
  for (int i = 0; i < kMaxCamSearch; ++i)
  {
    cv::VideoCapture enumCap;
    if (enumCap.open(i, kPREF_API))
    {
      if (enumCap.isOpened())
      {
        cameras.emplace_back(i, kPREF_API, "", "", nullptr);
      }
      enumCap.release();
    }
  }
  return cameras;
}
#endif

#if _WIN32

// Enumeration callback
typedef std::function<void(const std::string& deviceName, const std::string& devicePath,
                           IMFActivate* devActivate)>
    WinEnumCallback;

std::string GetDeviceString(IMFActivate* device, const GUID& guid)
{
  std::string value;
  WCHAR* buffer = nullptr;
  UINT32 bufLen = 255;
  if (FAILED(device->GetAllocatedString(guid, &buffer, &bufLen)) || (buffer == nullptr))
  {
    return value;
  }
  value = WideToString(buffer);
  CoTaskMemFree(buffer);
  return value;
}

void EnumerateCamerasMf(WinEnumCallback callback)
{
  // choose device
  IMFAttributes* attribs = nullptr;

  if (FAILED(MFCreateAttributes(&attribs, 1)))
  {
    ZEBRAL_THROW("Failed to create MF Attributres", Result::ZBA_SYS_ATT_ERROR);
  }

  AutoRel<IMFAttributes> atts(attribs);
  if (FAILED(atts->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                           MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)))
  {
    ZEBRAL_THROW("Failed to setup MF Attributes", Result::ZBA_SYS_ATT_ERROR);
  }

  IMFActivate** devices = NULL;
  UINT32 count          = 0;
  if (FAILED(MFEnumDeviceSources(atts.get(), &devices, &count)))
  {
    ZEBRAL_THROW("Failed to enumerate devices", Result::ZBA_SYS_ATT_ERROR);
  }

  AutoRelArray<IMFActivate> deviceArray(devices, count);
  for (size_t i = 0; i < count; ++i)
  {
    std::string deviceName = GetDeviceString(deviceArray[i], MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME);
    std::string devicePath =
        GetDeviceString(deviceArray[i], MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK);

    if (deviceName.empty())
    {
      continue;
    }

    if (callback)
    {
      callback(deviceName, devicePath, devices[i]);
    }
  }
}

#endif  // _WIN32

/// Get a list of camera info structures
std::vector<CameraInfo> CameraManager::Enumerate()
{
  std::vector<CameraInfo> cameras;
#ifdef _WIN32
  // Callback during enumeration to add CameraInfo devices to cameras_
  // (we'll reuse the enumerate to find requested devices and open them)
  auto enumNames = [&](const std::string& devName, const std::string& devPath, IMFActivate*) {
    uint16_t vid = 0;
    uint16_t pid = 0;
    if (!devPath.empty())
    {
      std::regex vidPidRe("usb#vid_([0-9a-z]{4})&pid_([0-9a-z]{4})",
                          std::regex_constants::ECMAScript | std::regex_constants::icase);

      std::smatch matches;
      if (std::regex_search(devPath, matches, vidPidRe))
      {
        if (matches.size() == 3)
        {
          vid =
              static_cast<uint16_t>(std::stoul(std::string("0x") + matches[1].str(), nullptr, 16));
          pid =
              static_cast<uint16_t>(std::stoul(std::string("0x") + matches[2].str(), nullptr, 16));
        }
      }
    }

    // Don't save the activation during normal enumeration,
    // since we'll delete it before they can use it.
    cameras.emplace_back(static_cast<int>(cameras.size()), 0, devName, devPath, nullptr, vid, pid);
  };
  EnumerateCamerasMf(enumNames);
#else
  cameras = EnumerateOpenCV();
#endif
  return cameras;
}

/// Create a camera with an information structure from Enumerate
std::shared_ptr<Camera> CameraManager::Create(const CameraInfo& info)
{
#if _WIN32
  std::shared_ptr<Camera> camera;
  auto createCallback = [&](const std::string& devName, const std::string& devPath,
                            IMFActivate* device) {
    if ((devName == info.name) && (devPath == info.path))
    {
      camera = std::make_shared<CameraWin>(CameraInfo(info.index, 0, devName, devPath, device));
    }
  };
  EnumerateCamerasMf(createCallback);
  return camera;
#else
  return std::shared_ptr<Camera>(new CameraOpenCV(info));
#endif
}

}  // namespace zebral