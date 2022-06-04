/// \file camera_info.hpp
/// Camera and format information structs

#ifndef LIGHTBOX_CAMERA_CAMERA_INFO_HPP_
#define LIGHTBOX_CAMERA_CAMERA_INFO_HPP_

#include <ostream>
#include <string>
#include <vector>

namespace zebral
{
/// Generic OSHandle for creation. Using this so we don't have to pollute everything
typedef void* OSHANDLE;

/// Video format information.
struct FormatInfo
{
  FormatInfo(int fmt_index = 0, int fmt_width = 0, int fmt_height = 0, float fmt_fps = 0.0f,
             int fmt_channels = 0, int fmt_bitdepth = 0, int fmt_stride = 0,
             const std::string& fmt_format = "")
      : index(fmt_index),
        width(fmt_width),
        height(fmt_height),
        fps(fmt_fps),
        channels(fmt_channels),
        bitdepth(fmt_bitdepth),
        stride(fmt_stride),
        format(fmt_format)
  {
  }

  int index;
  int width;
  int height;
  float fps;
  int channels;
  int bitdepth;
  int stride;
  std::string format;
};

/// Information about a camera gathered from enumeration via CameraMgr
struct CameraInfo
{
  static constexpr int NO_FORMAT_SELECTED = -1;

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

std::ostream& operator<<(std::ostream& os, const CameraInfo& camInfo);

}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_CAMERA_INFO_HPP_