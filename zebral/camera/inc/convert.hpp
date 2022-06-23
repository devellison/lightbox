/// \file convert.hpp
/// Generic video conversion routines
#ifndef LIGHTBOX_CAMERA_CONVERT_HPP_
#define LIGHTBOX_CAMERA_CONVERT_HPP_

#include <algorithm>

namespace zebral
{
class CameraFrame;

#pragma pack(push, 1)
struct fmt_YUY2
{
  uint8_t y0;
  uint8_t u;
  uint8_t y1;
  uint8_t v;
};
struct fmt_BGR8
{
  uint8_t b;
  uint8_t g;
  uint8_t r;
};

struct fmt_BGRA
{
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t a;
};

struct fmt_NV12_UV
{
  uint8_t u;
  uint8_t v;
};

struct fmt_NV12_Y
{
  uint8_t y;
};

#pragma pack(pop)

template <class T>
uint8_t Clamp8bit(T value)
{
  T kMin8 = 0;
  T kMax8 = 255;
  return static_cast<uint8_t>(std::clamp(value, kMin8, kMax8));
}

/// Definition for pixel-wise yuv to rgb function
typedef void (*YUVRGBFUNC)(uint8_t y, uint8_t u, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b);

/// You can point this to a different conversion func if desired.
/// Especially for speed checking or accuracy checking.
/// Defaults to fixed-point for decent speed, although not
/// nearly what you can get if you do it in larger chunks with SIMD.
extern YUVRGBFUNC YUV2RGB;

/// raw yuv to RGB conversion
/// right now this is a slow reference version
void YUVToRGB(uint8_t y, uint8_t u, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b);

/// Fixed point - a bit faster.
void YUVToRGBFixed(uint8_t y, uint8_t u, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b);

/// Convert a row of YUY2
void YUY2ToBGRRow(const uint8_t* src, uint8_t* dst, int width);

/// Convert a row of NV12
void NV12ToBGRRow(const uint8_t* src_ptr_y, const uint8_t* src_ptr_uv, uint8_t* dst_ptr, int width);

/// Converts a row of BGRA
void BGRAToBGRRow(const uint8_t* src, uint8_t* dst, int width);

/// Converts a frame of YUY2 into an existing CameraFrame
void YUY2ToBGRFrame(const uint8_t* src, CameraFrame& frame, int stride);
/// Converts a frame of NV12 into an existing CameraFrame
void NV12ToBGRFrame(const uint8_t* src, CameraFrame& frame, int stride);
/// Converts BGRA to BGR in an existing frame
void BGRAToBGRFrame(const uint8_t* src, CameraFrame& frame, int stride);

/// Creates a frame and converts YUY2 into it
CameraFrame YUY2ToBGRFrame(const uint8_t* src, int width, int height, int stride);
/// Creates a frame and converts NV12 into it
CameraFrame NV12ToBGRFrame(const uint8_t* src, int width, int height, int stride);
/// Creates a frame and converts BGRA into it
CameraFrame BGRAToBGRFrame(const uint8_t* src, int width, int height, int stride);

void GreyRow(const uint8_t* src, uint8_t* dst, int stride);
void GreyToFrame(const uint8_t* src, CameraFrame& out, int stride);
CameraFrame Grey16ToFrame(const uint8_t* src, int width, int height, int stride);
CameraFrame Grey8ToFrame(const uint8_t* src, int width, int height, int stride);

}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_CONVERT_HPP_