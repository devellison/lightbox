/// \file store_error.cpp
/// Implementation of errno storage
#include "store_error.hpp"
#include "errors.hpp"

#include <errno.h>
#include <string.h>
#if _WIN32
#include <windows.h>
#endif
namespace zebral
{
StoreError::StoreError()
{
  last_error_ = errno;
}
StoreError::~StoreError()
{
  errno = last_error_;
}
int StoreError::Get() const
{
  return last_error_;
}

std::string StoreError::ToString() const
{
  return SysErrorToString(last_error_);
}

}  // namespace zebral