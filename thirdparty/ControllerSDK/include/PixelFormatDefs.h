/*!
 * \file PixelFormatDefs.h
 * \date 2020/05/30 15:24
 *
 * \author Von
 * Contact: fenghezhou1@huya.com
 *
 * \brief
 *
 * TODO: long description
 *
 * \note
*/

// make sure file included only once
#ifndef PIXELFORMATDEFS_H_
#define PIXELFORMATDEFS_H_

// 像素格式,pipeline/pipeline与插件的接口中统一使用这个格式
// MediaClient等地方已经假定值不可超过255
enum PixelFormat
{
  kPixelFormat_BGR24 = 0,
  kPixelFormat_BGRA = 1,
  kPixelFormat_RGB24 = 2,
  kPixelFormat_RGBA = 3,
  // B10G10R10A2
  kPixelFormat_30RA = 4,
  // R10G10B10A2
  kPixelFormat_30BA = 5,
  kPixelFormat_NV12 = 6,
  kPixelFormat_NV21 = 7,
  kPixelFormat_YUV420 = 8,
  kPixelFormat_RAW = 9,//裸数据透传
  kPixelFormat_Count,
  kPixelFormat_Max = 255
};

#endif  // PIXELFORMATDEFS_H_
