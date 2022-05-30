/// \file log.hpp
/// Logging and tracing functions

#ifndef LIGHTBOX_CAMERA_LOG_HPP_
#define LIGHTBOX_CAMERA_LOG_HPP_
#include <cstdarg>
#include <iostream>
#include <sstream>
#include <string>
#include "platform.hpp"

namespace zebral
{
/// Number of digits to use for seconds precision
static constexpr int ZEBRAL_LOG_PRECISION = 4;

#define ZEBRAL_EXPSTRING(x)  #x                   ///< line-expansion for ZEBRAL_LOCINFO
#define ZEBRAL_STRINGCONV(x) ZEBRAL_EXPSTRING(x)  ///< line-expansion for ZEBRAL_LOCINFO

/// Convert location in source code to string
#define ZEBRAL_LOCINFO __FILE__ "(" ZEBRAL_STRINGCONV(__LINE__) ") : "

#define ZEBRAL_TODO(x) message(ZEBRAL_LOCINFO x)  ///< Usage: #pragma REMINDER("Fix this")

/// Log level, ad them as we need.
/// 0 goes to std::cerr, others to std::cout right now.
enum class ZEBRAL_LL : int
{
  LL_ERROR,  ///< Error Log level (stderr)
  LL_INFO    ///< Info log level (stdout)
};

/// Normal logging macro - goes to stdout
/// \param msg - message and printf style formatting string
/// \param ... - printf style arguments
#define ZEBRAL_LOG(msg, ...) zebral_log(ZEBRAL_LL::LL_INFO, ZEBRAL_LOCINFO, msg, ##__VA_ARGS__)

/// Error logging macro - goes to sterr
/// \param msg - message and printf style formatting string
/// \param ... - printf style arguments
#define ZEBRAL_ERR(msg, ...) zebral_log(ZEBRAL_LL::LL_ERROR, ZEBRAL_LOCINFO, msg, ##__VA_ARGS__)

/// This logs types that have an operator<< overload but not a string conversion.
/// \param obj - object to log, must have operator<< overload
#define ZEBRAL_LOGSS(obj) zebral_logss(ZEBRAL_LL::LL_INFO, ZEBRAL_LOCINFO, obj)

/// Assert with message. Right now, works in both debug and release modes.
/// May add removal later?  Also a retry for debugging like CAT had?
#define ZEBRAL_ASSERT(cond, msg)                     \
  if (!(cond))                                       \
  {                                                  \
    ZEBRAL_THROW(msg, Result::ZBA_ASSERTION_FAILED); \
  }

/// base log function - use ZEBRAL_LOG, ZEBRAL_ERR, ZEBRAL_LOGSS macros.
/// \param level - log level. Right now, LL_ERROR goes to stderr, everything else to stdout
/// \param loc - ZERBRAL_LOCINFO file(line) string
/// \param msg - log message
/// \param ... - variable arguments formatted as printf()
void zebral_log(ZEBRAL_LL level, const char* loc, const char* msg, ...);

/// stream log function - logs classes that have an operator<< overload but not an
/// easy const char* out.
/// \tparam T - class type to log
/// \param level - log level
/// \param loc - ZEBRAL_LOCINFO string for source location
/// \param T - class instance to log
template <class T>
void zebral_logss(ZEBRAL_LL level, const char* loc, T msg)
{
  std::stringstream ss;
  ss << msg;
  zebral_log(level, loc, ss.str().c_str());
}

/// {TODO} Replace all this with std::fmt when c++20 is more available.

/// sprintf to a std::string
/// \param msg - message with printf-style formatters
/// \param ... - variable arguments for printf() style formatting
std::string zebral_sformat(const char* msg, ...);

/// sprintf to a std::string taking a var argument list
/// \param msg - message with printf-style formatters
/// \param vaArgs - variable arguments for printf() style formatting
std::string zebral_sformat(const char* msg, va_list vaArgs);
}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_LOG_HPP_