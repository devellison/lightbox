/// \file errors.hpp
/// Error Result codes, exceptions, handling

#ifndef LIGHTBOX_ERRORS_HPP_
#define LIGHTBOX_ERRORS_HPP_

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace zebral
{
static constexpr size_t kMaxErrorLength = 256;
/// Error codes, often passed with exceptions.
/// Expect lightbox to be small.... if not, this needs TLC.
enum class Result : uint32_t
{
  ZBA_SUCCESS       = 0,  ///< General success result
  ZBA_STATUS        = 1,  ///< Positive values (no high bit set) are successful, but with info
  ZBA_UNKNOWN_ERROR = 0xFFFFFFFF,  ///< negative values are errors

  ZBA_ERROR                = 0x80000000,  ///< General errors
  ZBA_UNDEFINED_VALUE      = 0x80000001,
  ZBA_INVALID_COMMAND_LINE = 0x80000003,
  ZBA_ASSERTION_FAILED     = 0x80000004,
  ZBA_INVALID_RANGE        = 0x80000005,

  ZBA_CAMERA_ERROR       = 0x80001000,  ///< Camera errors
  ZBA_CAMERA_OPEN_FAILED = 0x80001001,
  ZBA_UNSUPPORTED_FMT    = 0x80001002,

  ZBA_SYS_ERROR     = 0x80002000,  ///< System errors
  ZBA_SYS_COM_ERROR = 0x80002001,
  ZBA_SYS_MF_ERROR  = 0x80002002,
  ZBA_SYS_ATT_ERROR = 0x80002003

};

/// Return a result as an unsigned
uint32_t to_unsigned(Result result);

/// Return a result as an int (e.g. for executable error codes)
int to_int(Result result);

/// Right now, just export hex to convert a result to a string.
std::string to_string(Result result);

/// Returns true on negative results
bool Failed(Result result);

/// Returns true on SUCCESS (0) and positive status results
bool Success(Result result);

/// Sets up a handler for unhandled exceptions.  Call this once at startup.
void SetUnhandled();

/// Exception base class to hold error code + message
class Error : public std::runtime_error
{
 public:
  /// Generally, use ZBA_THROW or similar macro to throw these so it'll capture the file/line as
  /// well.
  Error(const std::string& msg, Result result = Result::ZBA_UNKNOWN_ERROR,
        const char* file = nullptr, int line = 0, int sys_errno = 0)
      : std::runtime_error(msg),
        result_(result),
        where_(std::string(file) + "(" + std::to_string(line) + ")"),
        errno_(sys_errno)
  {
  }

  virtual ~Error() = default;

  /// Return the error code (why it happened)
  Result why() const
  {
    return result_;
  }

  /// Return the location in the source (where it happened)
  std::string where() const
  {
    return where_;
  }

  /// Return the system error if one was given
  int system_error() const
  {
    return errno_;
  }

 protected:
  Result result_;      ///< Numeric result in case we want to handle things nicely.
  std::string where_;  ///< Where in the code the exception was thrown
  int errno_;          ///< ErrorNo at time of throw, if provided.
};
/// Error utility function - convert errno to string
std::string SysErrorToString(int errorCode);

/// Error utility function - convert zebral::Result codes to strings
std::string ZBAErrorToString(Result result);

/// Macro to throw an error with location
#define ZBA_THROW(msg, result) throw Error(msg, result, __FILE__, __LINE__)

/// Macro to throw an error with location and errorno
#define ZBA_THROW_ERRNO(msg, result) throw Error(msg, result, __FILE__, __LINE__, errno)

}  // namespace zebral
#endif  // LIGHTBOX_ERRORS_HPP_