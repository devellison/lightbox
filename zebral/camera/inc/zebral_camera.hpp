/// \file zebral_camera.hpp
/// Interface header for the ZeBrAl camera library.
#ifndef ZBA_CAMERA_HPP_
#define ZBA_CAMERA_HPP_

#ifndef ZBA_OPENCV_INTERFACE
#define ZBA_OPENCV_INTERFACE 0
#endif

#if ZBA_OPENCV_INTERFACE
#include "camera2cv.hpp"
#endif

#include "camera.hpp"
#include "camera_frame.hpp"
#include "camera_info.hpp"
#include "camera_manager.hpp"
#include "errors.hpp"

#endif  // ZBA_CAMERA_HPP_