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
  SUCCESS       = 0,
  STATUS        = 1,
  UNKNOWN_ERROR = 0xFFFFFFFF,
  ERROR         = 0x80000000,
  INVALID_COMMAND_LINE,
  CAMERA_ERROR = 0x80001000
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
  /// Generally, use ZEBRAL_THROW or similar macro to throw these so it'll capture the file/line as
  /// well.
  Error(const std::string& msg, Result result = Result::UNKNOWN_ERROR, const char* file = nullptr,
        int line = 0)
      : std::runtime_error(BuildMsg(msg, result, file, line)), result_(result)
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
#define ZEBRAL_THROW(msg, result) \
  throw Error(std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + msg, result)

}  // namespace zebral
#endif  // LIGHTBOX_ERRORS_HPP_