#ifdef _WIN32
#include "camera_win_impl.hpp"
#include "camera_frame.hpp"

namespace zebral
{
CameraWin::Impl::Impl(CameraWin *camera, IMFActivate *activate)
    : parent_(*camera),
      refCount_(1),
      activate_(activate),
      device_(nullptr),
      reader_(nullptr),
      width_(0),
      height_(0),
      stride_(0)
{
  // {TODO} I think? Need to verify..
  activate_->AddRef();
}

CameraWin::Impl::~Impl() {}

/// Initialize the device and reader and start capture.
void CameraWin::Impl::Start()
{
  // Reset formats
  parent_.info_.formats.clear();

  std::lock_guard<std::mutex> lock(mutex_);
  IMFMediaSource *device = nullptr;
  activate_->ActivateObject(__uuidof(IMFMediaSource), reinterpret_cast<void **>(&device));
  if (!device)
  {
    ZEBRAL_THROW("Failed activating camera", Result::ZBA_CAMERA_OPEN_FAILED);
  }
  device_.reset(device);

  IMFAttributes *attribs = nullptr;
  if (FAILED(MFCreateAttributes(&attribs, 3)))
  {
    ZEBRAL_THROW("Failed creating attribs while activating camera", Result::ZBA_CAMERA_OPEN_FAILED);
  }
  AutoRel<IMFAttributes> atts(attribs);

  if (FAILED(atts->SetUINT32(
          MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING /*MF_READWRITE_DISABLE_CONVERTERS*/, TRUE)))
  {
    ZEBRAL_THROW("Failed disabling converters", Result::ZBA_CAMERA_OPEN_FAILED);
  }

  if (FAILED(atts->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this)))
  {
    ZEBRAL_THROW("Failed setting callback", Result::ZBA_CAMERA_OPEN_FAILED);
  }

  IMFSourceReader *reader = nullptr;
  if (FAILED(MFCreateSourceReaderFromMediaSource(device_.get(), atts.get(), &reader)))
  {
    ZEBRAL_THROW("Failed creating reader", Result::ZBA_CAMERA_OPEN_FAILED);
  }
  reader_.reset(reader);

  /// First time through, pick a format and see if we can get it translated to RGB32.
  /// But keep enumerating to get all the formats available.
  IMFMediaType *mediaType = nullptr;
  AutoRel<IMFMediaType> medType;
  bool selectedType = false;

  for (DWORD index = 0;; index++)
  {
    if (FAILED(reader_->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, index,
                                           &mediaType)))
    {
      break;
    }
    medType.reset(mediaType);

    UINT32 w    = 0;
    UINT32 h    = 0;
    LONG stride = 0;
    MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &w, &h);
    mediaType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32 *)&stride);

    // {TODO} pick up more of the values for formatinfo
    parent_.info_.formats.emplace_back(index, w, h, 0, 0, stride, 0);

    /// Try to get a media format that works with our conversion/progressive request until
    /// we find one.
    if (!selectedType)
    {
      mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
      mediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
      if (SUCCEEDED(reader_->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0,
                                                 mediaType)))
      {
        selectedType                  = true;
        parent_.info_.selected_format = index;
      }
    }

    medType.reset(nullptr);
  }

  if (FAILED(reader_->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL,
                                 NULL)))
  {
    ZEBRAL_THROW("Failed reading first sample", Result::ZBA_CAMERA_OPEN_FAILED);
  }
}

/// Stop capture and release the reader and device.
void CameraWin::Impl::Stop()
{
  std::lock_guard<std::mutex> lock(mutex_);
  reader_.reset(nullptr);
  device_.reset(nullptr);
}

/// IMFSourceReaderCallback OnReadSample()
/// This is our buffer callback for new frames.
STDMETHODIMP CameraWin::Impl::OnReadSample(HRESULT status, DWORD /*streamIndex*/,
                                           DWORD /*streamFlags*/, LONGLONG /*timestamp*/,
                                           IMFSample *sample)
{
  std::lock_guard<std::mutex> lock(mutex_);

  IMFMediaBuffer *buffer = nullptr;

  if (SUCCEEDED(status))
  {
    if (sample)
    {
      if (width_ == 0)
      {
        IMFMediaType *mediaType = nullptr;
        if (FAILED(reader_->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                                &mediaType)))
        {
          ZEBRAL_THROW("Failed to get stream size", Result::ZBA_CAMERA_ERROR);
        }
        MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &width_, &height_);
        mediaType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32 *)&stride_);
      }

      // Get the video frame buffer from the sample.
      if (FAILED(sample->GetBufferByIndex(0, &buffer)))
      {
        ZEBRAL_THROW("Failed to get buffer!", Result::ZBA_CAMERA_ERROR);
      }
      AutoRel<IMFMediaBuffer> mediaBuf(buffer);

      BYTE *locked = nullptr;
      buffer->Lock(&locked, NULL, NULL);
      // Got buffer, copy it out and unlock it.
      // Maybe we shouldn't do a copy? Need to test speeds copy vs unlocking late.
      CameraFrame raw(width_, height_, 4, 1, false, true, reinterpret_cast<uint8_t *>(locked));
      buffer->Unlock();

      parent_.OnFrameReceived(raw);
    }
  }

  // Ask for the next frame and exit
  return reader_->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, nullptr,
                             nullptr, nullptr);
}

/// IMFSourceReaderCallback OnEvent()
STDMETHODIMP CameraWin::Impl::OnEvent(DWORD, IMFMediaEvent *)
{
  return S_OK;
}

/// IMFSourceReaderCallback OnFlush()
STDMETHODIMP CameraWin::Impl::OnFlush(DWORD)
{
  return S_OK;
}

/// IUnknown QueryInterface
STDMETHODIMP CameraWin::Impl::QueryInterface(REFIID riid, void **ppvObject)
{
  if (!ppvObject) return E_POINTER;

  if (__uuidof(IUnknown) == riid)
    *ppvObject = (IUnknown *)(IMFSourceReaderCallback *)this;
  else if (__uuidof(IMFSourceReaderCallback) == riid)
    *ppvObject = (IMFSourceReaderCallback *)this;
  else
  {
    *ppvObject = 0;
    return E_NOINTERFACE;
  }

  AddRef();
  return S_OK;
}

/// IUnknown AddRef
STDMETHODIMP_(ULONG) CameraWin::Impl::AddRef()
{
  return InterlockedIncrement(&refCount_);
}

/// IUnknown Release()
STDMETHODIMP_(ULONG) CameraWin::Impl::Release()
{
  ULONG count = InterlockedDecrement(&refCount_);
  if (!count) delete this;
  return count;
}

}  // namespace zebral
#endif  // _WIN32