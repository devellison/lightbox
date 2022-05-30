#ifndef LIGHTBOX_CAMERA_INFO_HPP_
#define LIGHTBOX_CAMERA_INFO_HPP_

#include <opencv2/opencv.hpp>
#include <ostream>
#include <string>

namespace zebral
{
/// Information about a camera gathered from enumeration via CameraMgr
struct CameraInfo
{
  CameraInfo(int idx, int api_val) : index(idx), api(api_val) {}
  // OpenCV index and API
  int index;
  int api;
};

std::ostream& operator<<(std::ostream& os, const CameraInfo& camInfo);

}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_INFO_HPP_