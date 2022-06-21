/// \file store_error.hpp
/// Utility class for storing / restoring errorno
#ifndef LIGHTBOX_CAMERA_STORE_ERROR_HPP_
#define LIGHTBOX_CAMERA_STORE_ERROR_HPP_

#include <string>

namespace zebral
{
/// Class to store an errno so we don't lose it in our debugging/logging functions
class StoreError final
{
 public:
  /// ctor - stores current errno
  StoreError();

  /// dtor - restores errorno
  ~StoreError();

  /// Converts the stored errno to a string
  /// \returns std::string for the errno
  std::string ToString() const;

  /// Retrieves the stored errorno
  /// \returns int errorno at time of construction
  int Get() const;

 private:
  int last_error_;  ///< Errorno value captured when constructed.
};
}  // namespace zebral
#endif