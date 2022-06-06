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
static constexpr int ZBA_LOG_PRECISION = 4;

#define ZBA_EXPSTRING(x)  #x                ///< line-expansion for ZBA_LOCINFO
#define ZBA_STRINGCONV(x) ZBA_EXPSTRING(x)  ///< line-expansion for ZBA_LOCINFO

/// Convert location in source code to string
#define ZBA_LOCINFO __FILE__ "(" ZBA_STRINGCONV(__LINE__) ") : "

#define ZBA_TODO(x) message(ZBA_LOCINFO x)  ///< Usage: #pragma REMINDER("Fix this")

/// Log level, ad them as we need.
/// 0 goes to std::cerr, others to std::cout right now.
enum class ZBA_LL : int
{
  LL_ERROR,  ///< Error Log level (stderr)
  LL_INFO    ///< Info log level (stdout)
};

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

/// Normal logging macro - goes to stdout
/// \param msg - message and printf style formatting string
/// \param ... - printf style arguments
#define ZBA_LOG(msg, ...) zba_log(ZBA_LL::LL_INFO, ZBA_LOCINFO, msg, ##__VA_ARGS__)

/// Error logging macro - goes to sterr
/// \param msg - message and printf style formatting string
/// \param ... - printf style arguments
#define ZBA_ERR(msg, ...) zba_log(ZBA_LL::LL_ERROR, ZBA_LOCINFO, msg, ##__VA_ARGS__)

#ifdef __clang__
#pragma clang diagnostic pop
#endif

/// This logs types that have an operator<< overload but not a string conversion.
/// \param obj - object to log, must have operator<< overload
#define ZBA_LOGSS(obj) zba_logss(ZBA_LL::LL_INFO, ZBA_LOCINFO, obj)

/// Assert with message. Right now, works in both debug and release modes.
/// May add removal later?  Also a retry for debugging like CAT had?
#define ZBA_ASSERT(cond, msg)                     \
  if (!(cond))                                    \
  {                                               \
    ZBA_THROW(msg, Result::ZBA_ASSERTION_FAILED); \
  }

/// base log function - use ZBA_LOG, ZBA_ERR, ZBA_LOGSS macros.
/// \param level - log level. Right now, LL_ERROR goes to stderr, everything else to stdout
/// \param loc - ZERBRAL_LOCINFO file(line) string
/// \param msg - log message
/// \param ... - variable arguments formatted as printf()
void zba_log(ZBA_LL level, const char* loc, const char* msg, ...);

/// stream log function - logs classes that have an operator<< overload but not an
/// easy const char* out.
/// \tparam T - class type to log
/// \param level - log level
/// \param loc - ZBA_LOCINFO string for source location
/// \param msg - class instance to log
template <class T>
void zba_logss(ZBA_LL level, const char* loc, T msg)
{
  std::stringstream ss;
  ss << msg;
  zba_log(level, loc, ss.str().c_str());
}

/// {TODO} Replace all this with std::fmt when c++20 is more available.

/// sprintf to a std::string
/// \param msg - message with printf-style formatters
/// \param ... - variable arguments for printf() style formatting
std::string zba_sformat(const char* msg, ...);

/// sprintf to a std::string taking a var argument list
/// \param msg - message with printf-style formatters
/// \param vaArgs - variable arguments for printf() style formatting
std::string zba_sformat(const char* msg, va_list vaArgs);
}  // namespace zebral

#include <string_view>

#define ZBA_TYPE_NAME(x) std::string(type_name<decltype(x)>()).c_str()
/// Utility to determine type of a variable
/// https://stackoverflow.com/questions/81870/is-it-possible-to-print-a-variables-type-in-standard-c/20170989#20170989
template <typename T>
constexpr auto type_name()
{
  std::string_view name, prefix, suffix;
#ifdef __clang__
  name   = __PRETTY_FUNCTION__;
  prefix = "auto type_name() [T = ";
  suffix = "]";
#elif defined(__GNUC__)
  name   = __PRETTY_FUNCTION__;
  prefix = "constexpr auto type_name() [with T = ";
  suffix = "]";
#elif defined(_MSC_VER)
  name   = __FUNCSIG__;
  prefix = "auto __cdecl type_name<";
  suffix = ">(void)";
#endif
  name.remove_prefix(prefix.size());
  name.remove_suffix(suffix.size());
  return name;
}

#endif  // LIGHTBOX_CAMERA_LOG_HPP_