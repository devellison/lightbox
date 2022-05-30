#ifndef LIGHTBOX_PLATFORM_HPP_
#define LIGHTBOX_PLATFORM_HPP_

#include <opencv2/opencv.hpp>
#if _WIN32
#include <windows.h>
#endif

namespace zebral
{
#if _WIN32
static const auto kPreferredVideoApi = cv::CAP_DSHOW;
#else
static const auto kPreferredVideoApi = cv::CAP_V4L2;
#endif
}  // namespace zebral

#endif  // LIGHTBOX_PLATFORM_HPP_