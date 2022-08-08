#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "args.hpp"
#include "camera.hpp"
#include "camera2cv.hpp"
#include "camera_http.hpp"
#include "camera_manager.hpp"
#include "config.hpp"
#include "errors.hpp"
#include "lightbox.hpp"
#include "log.hpp"
#include "nano_inc.hpp"
#include "param.hpp"
#include "platform.hpp"
#include "xml_factory.hpp"

using namespace zebral;
using namespace nanogui;
static const std::vector<ArgsConfigEntry> kArgTable = {{"help", '?', nullptr, "Show help"}};

int main(int argc, char** argv)
{
  Platform platform;
  Args args(argc, argv, kArgTable);

  if (args.has_errors())
  {
    args.display_errors();
    args.display_help("Usage: lightbox_app [OPTIONS]");
    return 1;
  }

  nanogui::init();

  auto lb = new Lightbox();
  lb->OpenCamera("http://10.0.0.22:81/video", "admin", "beer");
  nanogui::mainloop();

  delete lb;

  nanogui::shutdown();
  return 0;
}