#include "errors.hpp"

#if _WIN32
#include <windows.h>
#endif

namespace zebral
{
uint32_t to_unsigned(Result result)
{
  return static_cast<uint32_t>(result);
}

int to_int(Result result)
{
  return static_cast<int32_t>(result);
}

bool Failed(Result result)
{
  return (to_int(result) < 0);
}
bool Success(Result result)
{
  return (to_int(result) >= 0);
}

std::string to_string(Result result)
{
  std::stringstream ss;
  // Send hex to stringstream, then return result...
  ss << std::setfill('0') << std::setw(sizeof(uint32_t) * 2) << std::hex
     << static_cast<uint32_t>(result);
  return ss.str();
}

#if _WIN32
// Windows SEH handler
LONG WINAPI WinSehHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
  /// {TODO} Probably want to write a minidump or similar.
  std::cerr << "SEH Exception occurred: " << std::hex
            << pExceptionInfo->ExceptionRecord->ExceptionCode << std::endl;
  std::abort();
}
#endif  //_WIN32

// Right now just sorting exceptions a little because the Windows runtime wasn't printing
// anything on unhandled throws or other exceptions, just exiting silently.
// Later this could log troubleshooting information.
void SetUnhandled()
{
#if _WIN32
  SetUnhandledExceptionFilter(WinSehHandler);
#endif
  // Handle c++ errors and exit
  std::set_terminate([]() {
    std::cerr << "A fatal error has occurred." << std::endl;
    try
    {
      std::rethrow_exception(std::current_exception());
    }
    catch (const Error& e)
    {
      std::cerr << "Exception: " << e.what() << std::endl;

      // Exit with the Result code
      std::exit(to_int(e.result_));
    }
    catch (const std::runtime_error& e)
    {
      std::cerr << "Runtime Error: " << e.what() << std::endl;
    }
    catch (const std::exception& e)
    {
      std::cerr << "Unhandled exception: " << e.what() << std::endl;
    }

    std::abort();
  });
}

}  // namespace zebral