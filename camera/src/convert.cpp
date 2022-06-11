/// \file convert.cpp
/// Video conversion routines.. right now just plain C/C++
#include "convert.hpp"
#include <cmath>
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
// A great optimized implementation of all of this and more is
// Chromium's libyuv
// https://chromium.googlesource.com/libyuv/libyuv/
//
// Might also look at the table patents.....
// Not sure if memory or calcs will be better.
// "Fast filtered YUV to RGB conversion"
//
// https://patents.google.com/patent/US7639263B2/en
// "RAM based YUV - RGB conversion "
// https://patents.google.com/patent/US5872556A/en
//

// Reference implementation. Slow but accurate.
void YUVToRGB(uint8_t y, uint8_t u, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b)
{
  double y1 = y - 16;
  double u1 = u - 128;
  double v1 = v - 128;
  r         = Clamp8bit(std::lround(1.164 * y1 + 1.596 * v1));
  g         = Clamp8bit(std::lround(1.164 * y1 - 0.392 * u1 - 0.813 * v1));
  b         = Clamp8bit(std::lround(1.164 * y1 + 2.017 * u1));
}

void YUY2ToBGRRow(const uint8_t* src, uint8_t* dst, int width)
{
  const fmt_YUY2* yuy2 = reinterpret_cast<const fmt_YUY2*>(src);
  fmt_BGR8* bgr8       = reinterpret_cast<fmt_BGR8*>(dst);

  for (int x = 0; x < width - 1; x += 2)
  {
    YUVToRGB(yuy2->y0, yuy2->u, yuy2->v, bgr8->r, bgr8->g, bgr8->b);
    ++bgr8;
    YUVToRGB(yuy2->y1, yuy2->u, yuy2->v, bgr8->r, bgr8->g, bgr8->b);
    ++bgr8;
    yuy2++;
  }

  // Shouldn't see this unless we're cropping weird.
  if (width & 1)
  {
    YUVToRGB(yuy2->y0, yuy2->u, yuy2->v, bgr8->r, bgr8->g, bgr8->b);
  }
}

void NV12ToBGRRow(const uint8_t* src_ptr_y, const uint8_t* src_ptr_uv, uint8_t* dst_ptr, int width)
{
  const fmt_NV12_Y* nv12_y   = reinterpret_cast<const fmt_NV12_Y*>(src_ptr_y);
  const fmt_NV12_UV* nv12_uv = reinterpret_cast<const fmt_NV12_UV*>(src_ptr_uv);
  fmt_BGR8* bgr8             = reinterpret_cast<fmt_BGR8*>(dst_ptr);

  for (int x = 0; x < width - 1; x += 2)
  {
    YUVToRGB(nv12_y->y, nv12_uv->u, nv12_uv->v, bgr8->r, bgr8->g, bgr8->b);
    ++bgr8;
    ++nv12_y;
    YUVToRGB(nv12_y->y, nv12_uv->u, nv12_uv->v, bgr8->r, bgr8->g, bgr8->b);
    ++bgr8;
    ++nv12_y;
    // inc uv on alternating pixels
    ++nv12_uv;
  }

  if (width & 1)
  {
    YUVToRGB(nv12_y->y, nv12_uv->u, nv12_uv->v, bgr8->r, bgr8->g, bgr8->b);
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

}  // namespace zebral