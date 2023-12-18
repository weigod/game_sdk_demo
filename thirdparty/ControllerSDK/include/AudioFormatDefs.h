/*!
 * \file AudioFormatDefs.h
 * \date 2020/05/30 16:04
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
#ifndef AUDIOFORMATDEFS_H_
#define AUDIOFORMATDEFS_H_

// 音频格式,pipeline/pipeline与插件的接口中统一使用这个格式
/*
// https://www.jianshu.com/p/8ead320c9832
以立体声为例:
// 非planar格式: 2个声道交替存放
LRLRLRLRLRLRLRLRLR

// 而planar格式: 2个声道的数据各自连续
LLLLLLRRRRRRRR
*/
// MediaClient等地方已经假定AudioFormat <= 255
enum AudioFormat
{
  kAudioFormat_S16 = 0,             ///< signed 16 bits, 这个值已经对外发布(AudioFormat),不要修改
  kAudioFormat_U8,                  ///< unsigned 8 bits
  kAudioFormat_S32,                 ///< signed 32 bits
  kAudioFormat_Float,               ///< float
  kAudioFormat_Double,              ///< double

  kAudioFormat_U8Planar,            ///< unsigned 8 bits, planar
  kAudioFormat_S16Planar,           ///< signed 16 bits, planar
  kAudioFormat_S32Planar,           ///< signed 32 bits, planar
  kAudioFormat_FloatPlanar,         ///< float, planar
  kAudioFormat_DoublePlanar,        ///< double, planar
  kAudioFormat_S64,                 ///< signed 64 bits
  kAudioFormat_S64Planar,           ///< signed 64 bits, planar
  kAudioFormat_Count,
  kAudioFormat_Max = 255
};

// LowFrequency Effects (LFE) channel
enum AudioChannelLayout
{
  kAudioChannelLayout_None = 0,

  // Front C
  kAudioChannelLayout_Mono,

  // Front L, Front R
  kAudioChannelLayout_Stereo,

  // Stereo L, Stereo R, LFE
  kAudioChannelLayout_2Point1,

  // Front L, Front R, Back C
  kAudioChannelLayout_2_1,

  // Front L, Front R, Front C
  kAudioChannelLayout_Surround,

  // Stereo L, Stereo R, Front C, LFE
  kAudioChannelLayout_3_1,

  // Front L, Front R, Front C, Back C
  kAudioChannelLayout_4_0,

  // Stereo L, Stereo R, Front C, Rear C, LFE
  kAudioChannelLayout_4_1,

  // Front L, Front R, Side L, Side R
  kAudioChannelLayout_2_2,

  // Front L, Front R, Back L, Back R
  kAudioChannelLayout_Quad,

  // Front L, Front R, Front C, Side L, Side R
  kAudioChannelLayout_5_0,

  // Front L, Front R, Front C, Side L, Side R, LFE
  kAudioChannelLayout_5_1,

  // Front L, Front R, Front C, Back L, Back R
  kAudioChannelLayout_5_0_Back,

  // Front L, Front R, Front C, Back L, Back R, LFE
  kAudioChannelLayout_5_1_Back,

  // Stereo L, Stereo R, Front C, Side L, Side R, Back C
  kAudioChannelLayout_6_0,

  // Stereo L, Stereo R, Side L, Side R, Front LofC, Front RofC
  kAudioChannelLayout_6_0_Front,

  // Stereo L, Stereo R, Front C, Rear L, Rear R, Rear C
  kAudioChannelLayout_Hexagonal,

  // Stereo L, Stereo R, Front C, LFE, Side L, Side R, Rear Center
  kAudioChannelLayout_6_1,

  // Stereo L, Stereo R, Front C, LFE, Back L, Back R, Rear Center
  kAudioChannelLayout_6_1_Back,

  // Stereo L, Stereo R, Side L, Side R, Front LofC, Front RofC, LFE
  kAudioChannelLayout_6_1_Front,

  // Front L, Front R, Front C, Side L, Side R, Back L, Back R
  kAudioChannelLayout_7_0,

  // Front L, Front R, Front C, Side L, Side R, Front LofC, Front RofC
  kAudioChannelLayout_7_0_Front,

  // Front L, Front R, Front C, LFE, Side L, Side R, Back L, Back R
  kAudioChannelLayout_7_1,

  // Front L, Front R, Front C, LFE, Side L, Side R, Front LofC, Front RofC
  kAudioChannelLayout_7_1_Wide,

  // Front L, Front R, Front C, LFE, Back L, Back R, Front LofC, Front RofC
  kAudioChannelLayout_7_1_Wide_Back,

  // Front L, Front R, Front C, Side L, Side R, Rear L, Back R, Back C.
  kAudioChannelLayout_Octagonal,

  // Stereo L, Stereo R
  kAudioChannelLayout_Stereo_Downmix,

  // Actual channel layout is specified in the bitstream 
  // and the actual channel count is unknown at Chromium media pipeline level 
  // (useful for audio pass-through mode).
  kAudioChannelLayout_Bitstream,

  kAudioChannelLayout_Count,

  kAudioChannelLayout_Max = 255
};

#endif  // AUDIOFORMATDEFS_H_
