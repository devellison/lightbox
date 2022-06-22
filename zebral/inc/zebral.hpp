#ifndef LIGHTBOX_ZEBRAL_ZEBRAL_HPP_
#define LIGHTBOX_ZEBRAL_ZEBRAL_HPP_

#if _WIN32
#ifdef ZEBRAL_EXPORTS
#define ZEBRAL_API __declspec(dllexport)
#else
#define ZEBRAL_API __declspec(dllimport)
#endif
#else
#define ZEBRAL_API
#endif

extern "C"
{
  ZEBRAL_API const char* ZebralVersion();
}

#endif  // LIGHTBOX_ZEBRAL_ZEBRAL_HPP_