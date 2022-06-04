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
/// Error codes, often passed with exceptions.
/// Expect lightbox to be small.... if not, this needs TLC.
enum class Result : uint32_t
{
  ZBA_SUCCESS       = 0,
  ZBA_STATUS        = 1,
  ZBA_UNKNOWN_ERROR = 0xFFFFFFFF,

  ZBA_ERROR                = 0x80000000,
  ZBA_UNDEFINED_VALUE      = 0x80000001,
  ZBA_INVALID_COMMAND_LINE = 0x80000003,
  ZBA_ASSERTION_FAILED     = 0x80000004,

  ZBA_CAMERA_ERROR       = 0x80001000,
  ZBA_CAMERA_OPEN_FAILED = 0x80001001,
  ZBA_UNSUPPORTED_FMT    = 0x80001002,

  ZBA_SYS_ERROR     = 0x80002000,
  ZBA_SYS_COM_ERROR = 0x80000001,
  ZBA_SYS_MF_ERROR  = 0x80000002,
  ZBA_SYS_ATT_ERROR = 0x80000003

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
        const char* file = nullptr, int line = 0)
      : std::runtime_error(BuildMsg(msg, result, file, line)),
        result_(result)
  {
  }

  virtual ~Error() = default;

  /// Build an exception message with the error code, file, and line.
  static std::string BuildMsg(const std::string& msg, Result result, const char* file, int line)
  {
    std::string fullmsg = msg + " (0x" + to_string(result) + ")";
    if (nullptr != file)
    {
      fullmsg += "@" + std::string(file) + "(" + std::to_string(line) + ")";
    }
    return fullmsg;
  }

  /// Numeric result in case we want to handle things nicely.
  Result result_;
};

/// Macro to throw an error with location
#define ZBA_THROW(msg, result) \
  throw Error(std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + msg, result)

}  // namespace zebral
#endif  // LIGHTBOX_ERRORS_HPP_