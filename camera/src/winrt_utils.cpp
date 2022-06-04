#ifdef _WIN32

#include <winrt/Windows.Networking.Connectivity.h>
#include <winrt/base.h>
#include <winrt_utils.hpp>

using namespace winrt;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;

std::vector<std::string> ZBA_GetHostNames()
{
  std::vector<std::string> names;
  /*
  for (auto const& name : NetworkInformation::GetHostNames())
  {
    names.emplace_back(name.ToString().c_str());
  }
  */
  return names;
}

#endif  // _WIN32