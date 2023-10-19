#ifndef CPP_EXPORT_H_
#define CPP_EXPORT_H_

// -------------------------------------------------------------------------
#if defined(WIN32)
#include <windows.h>
#if defined(CPP_IMPLEMENTATION)
  #define CPP_EXPORT __declspec(dllexport)
#else
  #define CPP_EXPORT __declspec(dllimport)
#endif  // defined(CPP_IMPLEMENTATION)

#else // defined(WIN32)
#if defined(CPP_IMPLEMENTATION)
  #define CPP_EXPORT __attribute__((visibility("default")))
#else
  #define CPP_EXPORT
#endif
#define __stdcall
#endif

// -------------------------------------------------------------------------
#endif /* CPP_EXPORT_H_ */
