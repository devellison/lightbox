#ifndef LIGHTBOX_CAMERA_AUTO_CLOSE_HPP_
#define LIGHTBOX_CAMERA_AUTO_CLOSE_HPP_

#include <functional>

namespace zebral
{
/// Auto-closing handle class.
/// Right now, just for file descriptors.
/// Template version worked great for MSVC/Clang11, but not so much for GCC9
class AutoClose
{
 public:
  static constexpr int InvalidValue = -1;
  /// ctor - accept a handle, closes on destruction
  /// \param handle - handle to manage
  AutoClose(int handle = InvalidValue);

  /// Destruct - closes handle if any
  ~AutoClose();

  /// Retrieves the handle
  /// \returns int - the handle
  int get() const;

  /// Closes current handle and set handle to new one
  /// \param new_handle - the new handle
  void reset(int new_handle = InvalidValue);

  /// Clears the object - closes handle if any.
  void clear();

  /// Releases the handle - returns it to you unclosed. Resets internal handle.
  /// \returns int - unclosed handle. You are responsible for closing it.
  int release();

  /// Returns true if the handle is valid/open
  /// \returns true if valid handle
  bool valid() const;

  /// Returns true if the handle is not valid
  /// \returns - true if invalid handle
  bool bad() const;

 private:
  int handle_;  ///< Handle from ::open() or similar to be closed via ::close()
};

}  // namespace zebral
#endif  //  LIGHTBOX_CAMERA_AUTO_CLOSE_HPP_