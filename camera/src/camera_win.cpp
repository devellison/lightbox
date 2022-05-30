#ifdef _WIN32

#include "camera_win.hpp"
#include "camera_win_impl.hpp"
#include "errors.hpp"
#include "log.hpp"
#include "platform.hpp"

namespace zebral
{
// CameraWin main class
// COM interfaces and most of the code
// is kept in implementation class below
CameraWin::CameraWin(const CameraInfo &info)
    : Camera(info),
      impl_(new Impl(this, reinterpret_cast<IMFActivate *>(info_.handle)))
{
}

CameraWin::~CameraWin() {}

void CameraWin::OnStart()
{
  impl_->Start();
}

void CameraWin::OnStop()
{
  impl_->Stop();
}

}  // namespace zebral

#endif  // _WIN32