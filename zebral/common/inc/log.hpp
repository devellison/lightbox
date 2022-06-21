/// \file log.hpp
/// Logging and tracing functions

#ifndef LIGHTBOX_CAMERA_LOG_HPP_
#define LIGHTBOX_CAMERA_LOG_HPP_
#include <cstdarg>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "platform.hpp"
#include "store_error.hpp"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

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
  LL_NONE,
  LL_ERROR,  ///< Error Log level (stderr)
  LL_INFO    ///< Info log level (stdout)
};

void ZBA_SetLogLevel(ZBA_LL logLevel);

/// Normal logging macro - goes to stdout
/// \param msg - message and printf style formatting string
/// \param ... - printf style arguments

#define ZBA_TIMER(name, msg, ...) \
  zba_stack_timer name(zba_source_loc::current(), msg __VA_OPT__(, ) __VA_ARGS__)
#define ZBA_LOG(msg, ...) \
  zba_log(ZBA_LL::LL_INFO, zba_source_loc::current(), msg __VA_OPT__(, ) __VA_ARGS__)

/// Error log
#define ZBA_ERR(msg, ...) \
  zba_log(ZBA_LL::LL_ERROR, zba_source_loc::current(), msg __VA_OPT__(, ) __VA_ARGS__)

/// Error log with errno
#define ZBA_ERRNO(msg, ...) \
  zba_log_errno(ZBA_LL::LL_ERROR, zba_source_loc::current(), msg __VA_OPT__(, ) __VA_ARGS__)

/// This logs types that have an operator<< overload but not a string conversion.
/// \param obj - object to log, must have operator<< overload
#define ZBA_LOGSS(obj) zba_logss(ZBA_LL::LL_INFO, zba_source_loc::current(), obj)

/// Assert with message. Right now, works in both debug and release modes.
/// May add removal later?  Also a retry for debugging like CAT had?
#define ZBA_ASSERT(cond, msg)                     \
  if (!(cond))                                    \
  {                                               \
    ZBA_THROW(msg, Result::ZBA_ASSERTION_FAILED); \
  }

/// Core log function - add file support etc when needed.
/// \param level - log level (LL_ERROR goes to std::cerr right now)
/// \param logstr - fully composed log string.
void zba_log_internal(ZBA_LL level, const std::string& logstr, const zba_source_loc& loc);

/// basic log function - use ZBA_LOG, ZBA_ERR, ZBA_LOGSS macros.
/// \param level - log level. Right now, LL_ERROR goes to stderr, everything else to stdout
/// \param loc - ZERBRAL_LOCINFO file(line) string
/// \param msg - log message (format-style)
/// \param ... - format-style args
template <typename... Args>
void zba_log(ZBA_LL level, const zba_source_loc& loc, const std::string& msg, Args&&... args)
{
  StoreError err;
  std::string msgstr = zba_vformat(msg, zba_make_args(args...));
  zba_log_internal(level, msgstr, loc);
}

template <typename... Args>
void zba_log_errno(ZBA_LL level, const zba_source_loc& loc, const std::string& msg, Args&&... args)
{
  StoreError err;
  std::string msgstr = zba_vformat(msg, zba_make_args(args...));
  std::string outstr = msgstr + " (" + err.ToString() + ") ";
  zba_log_internal(level, outstr, loc);
}

class zba_stack_timer
{
 public:
  template <typename... Args>
  zba_stack_timer(const zba_source_loc& loc, const std::string& msg, Args&&... args)
      : start_(zba_now()),
        start_loc_(loc)
  {
    log_msg_ = zba_vformat(msg, zba_make_args(args...));
    zba_log_internal(ZBA_LL::LL_INFO, log_msg_ + ": START", start_loc_);
  }

  double log(const std::string& msg)
  {
    auto elapsed_time = zba_elapsed_sec(start_);
    zba_log_internal(ZBA_LL::LL_INFO,
                     log_msg_ + ": " + msg + " (" + std::to_string(elapsed_time) + ")", start_loc_);
    return elapsed_time;
  }

  ~zba_stack_timer()
  {
    auto elapsed_time = zba_elapsed_sec(start_);
    zba_log_internal(ZBA_LL::LL_INFO, log_msg_ + ": END (" + std::to_string(elapsed_time) + ")",
                     start_loc_);
  }

 protected:
  std::string log_msg_;
  ZBA_TSTAMP start_;
  zba_source_loc start_loc_;
};
/// stream log function - logs classes that have an operator<< overload but not an
/// easy const char* out.
/// \tparam T - class type to log
/// \param level - log level
/// \param msg - class instance to log
template <class T>
void zba_logss(ZBA_LL level, const zba_source_loc& loc, T msg)
{
  std::stringstream ss;
  ss << msg;
  zba_log_internal(level, ss.str(), loc);
}

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

}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_LOG_HPP_