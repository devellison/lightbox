#include <string>
#include "camera_manager.hpp"
#include "camera_platform.hpp"
#include "log.hpp"
#include "param.hpp"

using namespace zebral;

struct Resolution
{
  Resolution(int width, int height) : x(width), y(height) {}

  bool operator<(const Resolution& p2) const
  {
    if (x != p2.x) return (x < p2.x);
    if (y != p2.y) return (y < p2.y);
    return false;
  }
  int x;
  int y;
};
std::ostream& operator<<(std::ostream& os, const Resolution& r1)
{
  os << "(" << r1.x << ", " << r1.y << ")";
  return os;
}

void enumerate()
{
  CameraManager mgr;
  auto infoList = mgr.Enumerate();
  for (auto curInfo : infoList)
  {
    std::cout << curInfo.name;

#if __linux__
    // The bus is valid on windows, but fugly.
    // linux the path is helpful.
    std::cout << " [" << curInfo.path << "] <" << curInfo.bus << ">";
#endif

    if (curInfo.vid)
    {
      std::cout << " (" << std::hex << curInfo.vid << ":" << std::hex << curInfo.pid << std::dec
                << ")";
    }
    std::cout << std::endl;
  }
}

void dump_controls()
{
  CameraManager mgr;
  auto infoList = mgr.Enumerate();
  for (auto curInfo : infoList)
  {
    auto camera = mgr.Create(curInfo);
    std::cout << curInfo.name << std::endl;

    auto paramNames = camera->GetParameterNames();
    for (const auto& curName : paramNames)
    {
      auto param = camera->GetParameter(curName);
      std::cout << param << std::endl;
    }

    std::cout << std::endl;
  }
}

void dump_all_modes()
{
  CameraManager mgr;
  auto infoList = mgr.Enumerate();
  for (auto curInfo : infoList)
  {
    auto camera = mgr.Create(curInfo);
    std::cout << curInfo.name << std::endl;
    auto modeList = camera->GetAllModes();
    for (auto curMode : modeList)
    {
      std::cout << zba_format("    ({},{}) {} @ {}fps", curMode.width, curMode.height,
                              curMode.format, curMode.fps)
                << std::endl;
    }
  }
}

void dump_supported_modes()
{
  CameraManager mgr;
  auto infoList = mgr.Enumerate();
  for (auto curInfo : infoList)
  {
    auto camera = mgr.Create(curInfo);
    auto info   = camera->GetCameraInfo();
    std::cout << curInfo.name << std::endl;
    for (auto curMode : info.formats)
    {
      std::cout << zba_format("    ({},{}) {} @ {}fps", curMode.width, curMode.height,
                              curMode.format, curMode.fps)
                << std::endl;
    }
  }
}

void dump_resolutions()
{
  CameraManager mgr;
  auto infoList = mgr.Enumerate();
  for (auto curInfo : infoList)
  {
    auto camera = mgr.Create(curInfo);
    std::cout << curInfo.name << std::endl;
    auto modeList = camera->GetAllModes();

    std::set<Resolution> avres;
    for (auto curMode : modeList)
    {
      avres.insert(Resolution(curMode.width, curMode.height));
    }
    for (auto curRes : avres)
    {
      std::cout << "    " << curRes << std::endl;
    }
  }
}

void dump_4ccs()
{
  CameraManager mgr;
  auto infoList = mgr.Enumerate();
  for (auto curInfo : infoList)
  {
    auto camera = mgr.Create(curInfo);
    std::cout << curInfo.name << std::endl;
    auto modeList = camera->GetAllModes();

    std::set<std::string> fourccs;
    for (auto curMode : modeList)
    {
      fourccs.insert(curMode.format);
    }
    for (auto cur4cc : fourccs)
    {
      std::cout << "    " << cur4cc << std::endl;
    }
  }
}

void test()
{
  // For this app, logging is kinda redundant
  ZBA_SetLogLevel(ZBA_LL::LL_INFO);
  CameraManager mgr;
  auto infoList = mgr.Enumerate();
  if (infoList.size() == 0) return;

  auto camera = mgr.Create(infoList[0]);
  auto info   = camera->GetCameraInfo();
  if (info.formats.size() == 0)
  {
    return;
  }

  camera->SetFormat(*info.formats.begin());

  camera->Start();
  auto frame = camera->GetNewFrame(10000);
  camera->Stop();

  if (frame)
  {
    std::cout << "Frame:" << *frame << std::endl;
  }
  else
  {
    std::cout << "Did not get a frame." << std::endl;
  }
}

int main(int argc, char** argv)
{
  // For this app, logging is kinda redundant
  ZBA_SetLogLevel(ZBA_LL::LL_NONE);
  Platform p;

  if (argc == 1)
  {
    std::cout << "Usage: zebra_camera_util [enum|res|4ccs|supported|allmodes|controls]"
              << std::endl;
  }
  for (int i = 1; i < argc; ++i)
  {
    if (std::strcmp(argv[i], "enum") == 0)
    {
      enumerate();
    }
    else if (std::strcmp(argv[i], "res") == 0)
    {
      dump_resolutions();
    }
    else if (std::strcmp(argv[i], "allmodes") == 0)
    {
      dump_all_modes();
    }
    else if (std::strcmp(argv[i], "supported") == 0)
    {
      dump_supported_modes();
    }
    else if (std::strcmp(argv[i], "4ccs") == 0)
    {
      dump_4ccs();
    }
    else if (std::strcmp(argv[i], "test") == 0)
    {
      test();
    }
    else if (std::strcmp(argv[i], "controls") == 0)
    {
      dump_controls();
    }
  }
  return 0;
}