/// \file camera_info.hpp
/// Camera and format information structs

#ifndef LIGHTBOX_CAMERA_CAMERA_INFO_HPP_
#define LIGHTBOX_CAMERA_CAMERA_INFO_HPP_

#include <ostream>
#include <set>
#include <string>
#include <vector>

namespace zebral
{
/// Returns the number of channels from a fourCC code, if we support it.
int ChannelsFromFourCC(const std::string& fmt_format);

/// Returns the number of bytes per channel per pixel from a fourCC code, if we support it.
int BytesPPPCFromFourCC(const std::string& fmt_format);

/// FourCC for arbitrary incoming strings.
uint32_t FourCCToUInt32(const std::string& fmt_format);

/// FourCC for statics (for switch/case and the like)
constexpr uint32_t FOURCCTOUINT32(char const format[5])
{
  return (format[0]) | (format[1] << 8) | (format[2] << 16) | (format[3] << 24);
}

/// Generic OSHandle for creation when needed. Using this so we don't have to pollute everything
typedef void* OSHANDLE;

/// Video format information.
struct FormatInfo
{
  /// Format information constructor
  /// \param fmt_width Width in pixels
  /// \param fmt_height Height in pixels
  /// \param fmt_fps Expected frames per second
  /// \param fmt_format Format string (FourCC usually)
  FormatInfo(int fmt_width = 0, int fmt_height = 0, float fmt_fps = 0.0f,
             const std::string& fmt_format = "")
      : width(fmt_width),
        height(fmt_height),
        fps(fmt_fps),
        channels(ChannelsFromFourCC(fmt_format)),
        bytespppc(BytesPPPCFromFourCC(fmt_format)),
        format(fmt_format)
  {
  }

  /// Less than comparison between FormatInfo for sorting.
  bool operator<(const FormatInfo& f) const;

  /// Returns true if the fields of the format struct match.
  /// 0 values are considered wildcards, so a fully blank
  /// FormatInfo will match anything.
  bool Matches(const FormatInfo& f) const;

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
  /// \param cam_name Name of camera
  /// \param cam_path Path / System identifier for camera
  /// \param os_handle OS-specific value (deprecated)
  /// \param vid If a USB device, then the VendorID. Else 0
  /// \param pid If a USB device, then the ProductID. Else 0
  CameraInfo(int cam_idx, const std::string& cam_name, const std::string& cam_bus,
             const std::string& cam_path, const std::string& cam_driver, uint16_t cam_vid = 0,
             uint16_t cam_pid = 0)
      : index(cam_idx),
        name(cam_name),
        bus(cam_bus),
        path(cam_path),
        driver(cam_driver),
        vid(cam_vid),
        pid(cam_pid),
        selected_format(NO_FORMAT_SELECTED)
  {
  }

  /// Add a format to the camera info
  /// \param format - FormatInfo to add to available formats for the camera.
  void AddFormat(const FormatInfo& format)
  {
    formats.emplace(format);
  }

  int index;           ///< Index
  std::string name;    ///< Friendly Device name (WinRt/V4l2)
  std::string bus;     ///< Device path (WinRt)
  std::string path;    ///< File path (V4L2 only - e.g. /dev/video0)
  std::string driver;  ///< Driver path (V4L2 only)
  uint16_t vid;        ///< Vendor ID (USB) if available. 0 otherwise.
  uint16_t pid;        ///< Product ID (USB) if available. 0 otherwise.

  std::set<FormatInfo> formats;  ///< Formats available
  int selected_format;  ///< index into formats of selected one, or NO_FORMAT_SELECTED for none.
};

/// Convenience operator for dumping CameraInfos for debugging
std::ostream& operator<<(std::ostream& os, const CameraInfo& camInfo);

/// Convenience operator for dumping FormatInfoss for debugging
std::ostream& operator<<(std::ostream& os, const FormatInfo& fmtInfo);

}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_CAMERA_INFO_HPP_