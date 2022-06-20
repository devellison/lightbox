/// \file log.hpp
/// Logging and debugging related functions
#include "log.hpp"
#include <iomanip>
#include <thread>

namespace zebral
{
// {TODO} Fix this.
static ZBA_LL gLogLevel = ZBA_LL::LL_INFO;
void ZBA_SetLogLevel(ZBA_LL logLevel)
{
  gLogLevel = logLevel;
}
void zba_log_internal(ZBA_LL level, const std::string& logstr, const zba_source_loc& loc)
{
  // yuck!
  uint64_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());

  // compose the rest of the log string
  std::string stampstr = zba_format("[{}] [{:x}] {}({}) : ", zba_local_time(zba_now(), 4),
                                    thread_id, loc.file_name(), loc.line());

  if (gLogLevel < level)
  {
    return;
  }

  // Output, right now just to stdout/stderr
  if (level == ZBA_LL::LL_ERROR)
  {
    std::cerr << stampstr << logstr << std::endl;
  }
  else
  {
    std::cout << stampstr << logstr << std::endl;
  }
}

}  // namespace zebral