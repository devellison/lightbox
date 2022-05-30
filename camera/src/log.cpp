#include "log.hpp"
#include <iomanip>
#include <thread>

namespace zebral
{
std::string zebral_sformat(const char* msg, ...)
{
  va_list vaArgs;
  va_start(vaArgs, msg);
  std::string outstr = zebral_sformat(msg, vaArgs);
  va_end(vaArgs);
  return outstr;
}

std::string zebral_sformat(const char* msg, va_list vaArgs)
{
  va_list vaCopy;

  // Get buffer size
  va_copy(vaCopy, vaArgs);
  int length = std::vsnprintf(NULL, 0, msg, vaCopy);
  va_end(vaCopy);

  // Allocate and format
  std::string buff;
  buff.resize(length + 1);
  std::vsnprintf(buff.data(), length + 1, msg, vaArgs);
  buff.resize(length);

  return buff;
}

void zebral_log(ZEBRAL_LL level, const char* loc, const char* msg, ...)
{
  if (!msg) return;

  // format message string
  va_list vaArgs;
  va_start(vaArgs, msg);
  std::string outstr = zebral_sformat(msg, vaArgs);
  va_end(vaArgs);

  // compose the rest of the log string
  std::stringstream ss;
  ss << "[" << zebral_local_time(zebral_now(), ZEBRAL_LOG_PRECISION) << "] ["
     << std::this_thread::get_id() << "] " << loc << outstr;

  // Output, right now just to stdout/stderr
  if (level == ZEBRAL_LL::LL_ERROR)
  {
    std::cerr << ss.str() << std::endl;
  }
  else
  {
    std::cout << ss.str() << std::endl;
  }
}
}  // namespace zebral