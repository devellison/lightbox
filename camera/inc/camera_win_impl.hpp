/// \file camera_win_impl.hpp
/// Windows camera implementation with Media Foundation api.
/// Don't include this directly - use camera_win.hpp instead.

#ifndef LIGHTBOX_CAMERA_CAMERA_WIN_IMPL_HPP_
#define LIGHTBOX_CAMERA_CAMERA_WIN_IMPL_HPP_
#ifdef _WIN32

#include <dshow.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <windows.h>
#include "camera_win.hpp"
#include "errors.hpp"
#include "log.hpp"
#include "platform.hpp"

namespace zebral
{
/// Internal platform specific class to keep
/// windows defs out of framework headers
class CameraWin::Impl : public IMFSourceReaderCallback
{
 public:
  Impl(CameraWin *camera, IMFActivate *activate);
  ~Impl();

  /// Initialize the device and reader and start capture.
  void Start();
  /// Stop capture and release the reader and device.
  void Stop();

  /// IMFSourceReaderCallback OnReadSample()
  /// This is our buffer callback for new frames.
  STDMETHODIMP OnReadSample(HRESULT status, DWORD /*streamIndex*/, DWORD /*streamFlags*/,
                            LONGLONG /*timestamp*/, IMFSample *sample);

  /// IMFSourceReaderCallback OnEvent()
  STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *);

  /// IMFSourceReaderCallback OnFlush()
  STDMETHODIMP OnFlush(DWORD);

  /// IUnknown QueryInterface
  STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);

  /// IUnknown AddRef
  STDMETHODIMP_(ULONG) AddRef();

  /// IUnknown Release
  STDMETHODIMP_(ULONG) Release();

 private:
  CameraWin &parent_;
  ULONG refCount_;
  AutoRel<IMFActivate> activate_;
  AutoRel<IMFMediaSource> device_;
  AutoRel<IMFSourceReader> reader_;
  UINT32 width_;
  UINT32 height_;
  LONG stride_;
  mutable std::mutex mutex_;
};

}  // namespace zebral
#endif  //_WIN32

#endif  // LIGHTBOX_CAMERA_CAMERA_WIN_IMPL_HPP_