#define ZEBRAL_EXPORTS
#include "zebral.hpp"
#include "platform.hpp"

static const char* ZEBRAL_VERSION = LIGHTBOX_VERSION;

const char* ZebralVersion()
{
  return ZEBRAL_VERSION;
}