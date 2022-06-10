#include "log.hpp"
#include <format>
#include <iomanip>
#include <thread>

namespace zebral
{
void zba_log_internal(ZBA_LL level, const std::string& logstr, const std::source_location& loc)
{
  // yuck!
  uint64_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());

  // compose the rest of the log string
  std::string stampstr = std::format("[{}] [{}] {}({}) : ", zba_local_time(zba_now()), thread_id,
                                     loc.file_name(), loc.line());

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