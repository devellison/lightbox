/// \file autorel_win.hpp
/// Auto-release objects for Windows COM

#ifndef LIGHTBOX_AUTOREL_WIN_HPP_
#define LIGHTBOX_AUTOREL_WIN_HPP_

#ifdef _WIN32
#include <windows.h>

namespace zebral
{
/// Auto-release for COM objects.
/// These are scoped only, not ref counted.
/// Trying to avoid ATL dependencies right now....
template <class T>
class AutoRel
{
 public:
  AutoRel(T *obj = nullptr) : obj_(obj) {}
  ~AutoRel()
  {
    reset(nullptr);
  }

  void reset(T *obj)
  {
    if (obj_) obj_->Release();
    obj_ = obj;
  }

  T *operator->() const
  {
    return obj_;
  }

  T *get() const
  {
    return obj_;
  }

  T *obj_;
};

/// Auto-release for COM arrays
/// Scoped only, not ref counted.
template <class T>
class AutoRelArray
{
 public:
  AutoRelArray(T **objArray = nullptr, size_t count = 0) : array_(objArray), count_(count) {}

  ~AutoRelArray()
  {
    reset(nullptr, 0);
  }

  void reset(T **objArray, size_t count)
  {
    if (array_)
    {
      for (auto i = 0; i < count_; ++i)
      {
        array_[i]->Release();
      }
      CoTaskMemFree(array_);
    }

    array_ = objArray;
    count_ = count;
  }

  T **operator->() const
  {
    return array_;
  }
  T *operator[](size_t index) const
  {
    return array_[index];
  }

  T **get() const
  {
    return array_;
  }

  T **array_;
  size_t count_;
};

}  // namespace zebral

#endif  // _WIN32
#endif  // LIGHTBOX_AUTOREL_WIN_HPP_