/* -------------------------------------------------------------------------
//	FileName		:	D:\yx_code\yx\yx_export.h
//	Creator			:	(zc) <zcnet4@gmail.com>
//	CreateTime	:	2016-11-1 14:21
//	Description	:	
//
// -----------------------------------------------------------------------*/
#ifndef CPP_EXPORT_H_
#define CPP_EXPORT_H_

// -------------------------------------------------------------------------

#if defined(WIN32)

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
#endif

// -------------------------------------------------------------------------
#endif /* CPP_EXPORT_H_ */
