/// \file camera_info.hpp
/// Camera and format information structs

#ifndef LIGHTBOX_CAMERA_CAMERA_INFO_HPP_
#define LIGHTBOX_CAMERA_CAMERA_INFO_HPP_

#include <ostream>
#include <string>
#include <vector>

namespace zebral
{
/// Generic OSHandle for creation when needed. Using this so we don't have to pollute everything
typedef void* OSHANDLE;

/// Video format information.
struct FormatInfo
{
  /// Format information constructor
  /// \param fmt_width Width in pixels
  /// \param fmt_height Height in pixels
  /// \param fmt_fps Expected frames per second
  /// \param fmt_channels Num channels
  /// \param fmt_bytespppc Bytes per pixel per channel
  /// \param fmt_format Format string (FourCC usually)
  FormatInfo(int fmt_width = 0, int fmt_height = 0, float fmt_fps = 0.0f, int fmt_channels = 0,
             int fmt_bytespppc = 0, const std::string& fmt_format = "")
      : width(fmt_width),
        height(fmt_height),
        fps(fmt_fps),
        channels(fmt_channels),
        bytespppc(fmt_bytespppc),
        format(fmt_format)
  {
  }

  int width;           ///< Width in pixels
  int height;          ///< Height in pixels
  float fps;           ///< Expected frames per second
  int channels;        ///< Num channels
  int bytespppc;       ///< Bytes per pixel per channel
  std::string format;  ///< Format string (FourCC usually)
};

/// Information about a camera gathered from enumeration via CameraMgr
struct CameraInfo
{
  /// No format selected
  static constexpr int NO_FORMAT_SELECTED = -1;

  /// CameraInfo ctor
  /// \param cam_idx Index of camera
  /// \param cam_api API (OpenCV only / deprecated)
  /// \param cam_name Name of camera
  /// \param cam_path Path / System identifier for camera
  /// \param os_handle OS-specific value (deprecated)
  /// \param vid If a USB device, then the VendorID. Else 0
  /// \param pid If a USB device, then the ProductID. Else 0
  CameraInfo(int cam_idx, int cam_api, const std::string& cam_name, const std::string& cam_path,
             OSHANDLE os_handle, uint16_t vid = 0, uint16_t pid = 0)
      : index(cam_idx),
        api(cam_api),
        name(cam_name),
        path(cam_path),
        handle(os_handle),
        vid(vid),
        pid(pid),
        selected_format(NO_FORMAT_SELECTED)
  {
  }

  /// Add a format to the camera info
  /// \param format - FormatInfo to add to available formats for the camera.
  void AddFormat(const FormatInfo& format)
  {
    formats.emplace_back(format);
  }

  int index;         ///< Index (OpenCV and Win DSHOW)
  int api;           ///< API (OpenCV)
  std::string name;  ///< Device name (Win DSHOW)
  std::string path;  ///< Device path (Win DSHOW)
  OSHANDLE handle;   ///< IMFActivate* for the device on Win
  uint16_t vid;      ///< Vendor ID (USB) if available. 0 otherwise.
  uint16_t pid;      ///< Product ID (USB) if available. 0 otherwise.

  std::vector<FormatInfo> formats;  ///< Formats available
  int selected_format;  ///< index into formats of selected one, or NO_FORMAT_SELECTED for none.
};

/// Convenience operator for dumping CameraInfos for debugging
std::ostream& operator<<(std::ostream& os, const CameraInfo& camInfo);

}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_CAMERA_INFO_HPP_