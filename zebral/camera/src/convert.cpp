/// \file convert.cpp
/// Video conversion routines.. right now just plain C/C++
#include "convert.hpp"
#include <cmath>
#include <memory>
#include "log.hpp"

namespace zebral
{
// YUV to RGB conversion....
//
// Basic reference here:
// https://web.archive.org/web/20180423091842/http://www.equasys.de/colorconversion.html
// Also:
// https://en.wikipedia.org/wiki/YUV
//
// A great optimized implementation of all of this and much more is
// Chromium's libyuv
// https://chromium.googlesource.com/libyuv/libyuv/

// Default to the faster of the single pixel functions
YUVRGBFUNC YUV2RGB = YUVToRGBFixed;

// Reference implementation. Slow but accurate.
void YUVToRGB(uint8_t y, uint8_t u, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b)
{
  double y1 = y - 16;
  double u1 = u - 128;
  double v1 = v - 128;
  b         = Clamp8bit(std::lround(1.164 * y1 + 2.017 * u1));
  g         = Clamp8bit(std::lround(1.164 * y1 - 0.392 * u1 - 0.813 * v1));
  r         = Clamp8bit(std::lround(1.164 * y1 + 1.596 * v1));
}

void YUVToRGBFixed(uint8_t y, uint8_t u, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b)
{
  // Do the above, only fixed point math and let the compiler optimize a bit.
  static constexpr int shift      = 16;
  static constexpr int multiplier = 1 << shift;
  static constexpr int half       = multiplier / 2;

  const int y1 = (y - 16) * static_cast<int>(1.164 * multiplier);
  const int u1 = (u - 128) * static_cast<int>(0.392 * multiplier);
  const int u2 = (u - 128) * static_cast<int>(2.017 * multiplier);
  const int v1 = (v - 128) * static_cast<int>(1.596 * multiplier);
  const int v2 = (v - 128) * static_cast<int>(0.813 * multiplier);

  r = Clamp8bit(((y1 + v1) + half) >> shift);
  g = Clamp8bit(((y1 - u1 - v2) + half) >> shift);
  b = Clamp8bit(((y1 + u2) + half) >> shift);
}

void YUY2ToBGRRow(const uint8_t* src, uint8_t* dst, int width)
{
  const fmt_YUY2* yuy2 = reinterpret_cast<const fmt_YUY2*>(src);
  fmt_BGR8* bgr8       = reinterpret_cast<fmt_BGR8*>(dst);

  for (int x = 0; x < width - 1; x += 2)
  {
    YUV2RGB(yuy2->y0, yuy2->u, yuy2->v, bgr8->r, bgr8->g, bgr8->b);
    ++bgr8;
    YUV2RGB(yuy2->y1, yuy2->u, yuy2->v, bgr8->r, bgr8->g, bgr8->b);
    ++bgr8;
    yuy2++;
  }

  // Shouldn't see this unless we're cropping weird.
  if (width & 1)
  {
    YUV2RGB(yuy2->y0, yuy2->u, yuy2->v, bgr8->r, bgr8->g, bgr8->b);
  }
}

void NV12ToBGRRow(const uint8_t* src_ptr_y, const uint8_t* src_ptr_uv, uint8_t* dst_ptr, int width)
{
  const fmt_NV12_Y* nv12_y   = reinterpret_cast<const fmt_NV12_Y*>(src_ptr_y);
  const fmt_NV12_UV* nv12_uv = reinterpret_cast<const fmt_NV12_UV*>(src_ptr_uv);
  fmt_BGR8* bgr8             = reinterpret_cast<fmt_BGR8*>(dst_ptr);

  for (int x = 0; x < width - 1; x += 2)
  {
    YUV2RGB(nv12_y->y, nv12_uv->u, nv12_uv->v, bgr8->r, bgr8->g, bgr8->b);
    ++bgr8;
    ++nv12_y;
    YUV2RGB(nv12_y->y, nv12_uv->u, nv12_uv->v, bgr8->r, bgr8->g, bgr8->b);
    ++bgr8;
    ++nv12_y;
    // inc uv on alternating pixels
    ++nv12_uv;
  }

  if (width & 1)
  {
    YUV2RGB(nv12_y->y, nv12_uv->u, nv12_uv->v, bgr8->r, bgr8->g, bgr8->b);
  }
}
void BGRAToBGRRow(const uint8_t* src, uint8_t* dst, int width)
{
  const fmt_BGRA* bgra = reinterpret_cast<const fmt_BGRA*>(src);
  fmt_BGR8* bgr8       = reinterpret_cast<fmt_BGR8*>(dst);

  for (int x = 0; x < width; ++x)
  {
    bgr8->b = bgra->b;
    bgr8->g = bgra->g;
    bgr8->r = bgra->r;
    bgra++;
    bgr8++;
  }
}

void YUY2ToBGRFrame(const uint8_t* src, CameraFrame& out, int stride)
{
  auto src_ptr   = src;
  auto dst_ptr   = out.data();
  int dst_stride = out.channels() * out.bytes_per_channel() * out.width();

  for (int y = 0; y < out.height(); ++y)
  {
    YUY2ToBGRRow(src_ptr, dst_ptr, out.width());
    src_ptr += stride;
    dst_ptr += dst_stride;
  }
}

void NV12ToBGRFrame(const uint8_t* src, CameraFrame& out, int stride)
{
  auto src_ptr_y  = src;
  auto src_ptr_uv = src + stride * out.height();

  auto dst_ptr   = out.data();
  int dst_stride = out.channels() * out.bytes_per_channel() * out.width();

  for (int y = 0; y < out.height(); ++y)
  {
    NV12ToBGRRow(src_ptr_y, src_ptr_uv, dst_ptr, out.width());
    src_ptr_y += stride;
    dst_ptr += dst_stride;
    // inc on alternating rows
    src_ptr_uv += (y & 1) * stride;
  }
}
void BGRAToBGRFrame(const uint8_t* src, CameraFrame& out, int stride)
{
  auto src_ptr   = src;
  auto dst_ptr   = out.data();
  int dst_stride = out.channels() * out.bytes_per_channel() * out.width();

  for (int y = 0; y < out.height(); ++y)
  {
    BGRAToBGRRow(src_ptr, dst_ptr, out.width());
    src_ptr += stride;
    dst_ptr += dst_stride;
  }
}

CameraFrame YUY2ToBGRFrame(const uint8_t* src, int width, int height, int stride)
{
  CameraFrame out(width, height, 3, 1, false, false);
  YUY2ToBGRFrame(src, out, stride);
  return out;
}

CameraFrame NV12ToBGRFrame(const uint8_t* src, int width, int height, int stride)
{
  CameraFrame out(width, height, 3, 1, false, false);
  NV12ToBGRFrame(src, out, stride);
  return out;
}
CameraFrame BGRAToBGRFrame(const uint8_t* src, int width, int height, int stride)
{
  CameraFrame out(width, height, 3, 1, false, false);
  BGRAToBGRFrame(src, out, stride);
  return out;
}

void GreyRow(const uint8_t* src, uint8_t* dst, int stride)
{
  std::memcpy(dst, src, stride);
}

void GreyToFrame(const uint8_t* src, CameraFrame& out, int stride)
{
  auto src_ptr   = src;
  auto dst_ptr   = out.data();
  int dst_stride = out.channels() * out.bytes_per_channel() * out.width();

  for (int y = 0; y < out.height(); ++y)
  {
    GreyRow(src_ptr, dst_ptr, dst_stride);
    src_ptr += stride;
    dst_ptr += dst_stride;
  }
}

CameraFrame Grey16ToFrame(const uint8_t* src, int width, int height, int stride)
{
  CameraFrame out(width, height, 2, 1, false, false);
  GreyToFrame(src, out, stride);
  return out;
}

CameraFrame Grey8ToFrame(const uint8_t* src, int width, int height, int stride)
{
  CameraFrame out(width, height, 1, 1, false, false);
  GreyToFrame(src, out, stride);
  return out;
}

}  // namespace zebral