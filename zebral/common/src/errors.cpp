/// \file errors.cpp
/// Utility functions for zebral::Result codes and exceptions
#include "errors.hpp"
#include <cstring>
#include <map>

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

// Without this, we should be largely old winapi free...
// But in testing it's annoying if running it terminates without a useful message.
#ifndef CATCH_UNHANDLED_SEH
#define CATCH_UNHANDLED_SEH 0
#endif

#if _WIN32
#if CATCH_UNHANDLED_SEH
#include <windows.h>
// Windows SEH handler
LONG WINAPI WinSehHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
  /// {TODO} Probably want to write a minidump or similar.
  std::cerr << "SEH Exception occurred: " << std::hex
            << pExceptionInfo->ExceptionRecord->ExceptionCode << std::endl;
  std::abort();
}
#endif  // CATCH_UNHANDLED_SEH
#endif  // _WIN32

// Right now just sorting exceptions a little because the Windows runtime wasn't printing
// anything on unhandled throws or other exceptions, just exiting silently.
// Later this could log troubleshooting information.
void SetUnhandled()
{
#if CATCH_UNHANDLED_SEH
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
      std::cerr << "Result: 0x" << std::hex << to_int(e.why()) << std::dec << std::endl;
      std::cerr << "At: " << e.where() << std::endl;
      // Exit with the Result code
      std::exit(to_int(e.why()));
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

std::string SysErrorToString(int errorCode)
{
  std::string error;
  if (errorCode == 0)
  {
    return error;
  }

  error.resize(kMaxErrorLength);

#if _WIN32
  strerror_s(error.data(), kMaxErrorLength, errorCode);
#else
  std::ignore = strerror_r(errorCode, error.data(), kMaxErrorLength);
#endif

  error.resize(strlen(error.c_str()));
  return error;
}

// Maintenance nightmare. Have a string table generate this stuff
// when time allows, or at least generate the header from this table so
// it's in one place.
std::string ZBAErrorToString(Result result)
{
  struct StringTblEntry
  {
    Result code;
    std::string codeName;
    std::string description;
  };

  const static std::map<Result, StringTblEntry> ZBAErrorTable = {
      {Result::ZBA_SUCCESS, {Result::ZBA_SUCCESS, "ZBA_SUCCESS", ""}},
      {Result::ZBA_STATUS, {Result::ZBA_STATUS, "ZBA_STATUS", ""}},
      {Result::ZBA_UNKNOWN_ERROR, {Result::ZBA_UNKNOWN_ERROR, "ZBA_UNKNOWN_ERROR", ""}},
      {Result::ZBA_ERROR, {Result::ZBA_ERROR, "ZBA_ERROR", ""}},
      {Result::ZBA_UNDEFINED_VALUE, {Result::ZBA_UNDEFINED_VALUE, "ZBA_UNDEFINED_VALUE", ""}},
      {Result::ZBA_INVALID_COMMAND_LINE,
       {Result::ZBA_INVALID_COMMAND_LINE, "ZBA_INVALID_COMMAND_LINE", ""}},
      {Result::ZBA_ASSERTION_FAILED, {Result::ZBA_ASSERTION_FAILED, "ZBA_ASSERTION_FAILED", ""}},
      {Result::ZBA_INVALID_RANGE, {Result::ZBA_INVALID_RANGE, "ZBA_INVALID_RANGE", ""}},
      {Result::ZBA_CAMERA_ERROR, {Result::ZBA_CAMERA_ERROR, "ZBA_CAMERA_ERROR", ""}},
      {Result::ZBA_CAMERA_OPEN_FAILED,
       {Result::ZBA_CAMERA_OPEN_FAILED, "ZBA_CAMERA_OPEN_FAILED", ""}},
      {Result::ZBA_UNSUPPORTED_FMT, {Result::ZBA_UNSUPPORTED_FMT, "ZBA_UNSUPPORTED_FMT", ""}},
      {Result::ZBA_SYS_ERROR, {Result::ZBA_SYS_ERROR, "ZBA_SYS_ERROR", ""}},
      {Result::ZBA_SYS_COM_ERROR, {Result::ZBA_SYS_COM_ERROR, "ZBA_SYS_COM_ERROR", ""}},
      {Result::ZBA_SYS_MF_ERROR, {Result::ZBA_SYS_MF_ERROR, "ZBA_SYS_MF_ERROR", ""}},
      {Result::ZBA_SYS_ATT_ERROR, {Result::ZBA_SYS_ATT_ERROR, "ZBA_SYS_ATT_ERROR", ""}}};
  auto iterator = ZBAErrorTable.find(result);
  if (iterator != ZBAErrorTable.end())
  {
    return iterator->second.codeName;
  }
  if (Failed(result))
  {
    return "Unknown Error";
  }
  return "Unknown Status";
}

}  // namespace zebral