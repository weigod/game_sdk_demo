#pragma once
#include "PixelFormatDefs.h"
#include "AudioFormatDefs.h"
#include "MediaDefs.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum cpp_event_key
{
  kEventKey_Receiver = 0, //收流事件
  kEventKey_Decoder = 1,  //解码事件
  kEventKey_Publisher = 2,//推流事件
  kEventKey_Pipeline = 3, //Pipeline进程事件
  kEventKey_PublisherAudioProgress = 4,//表示音频总体加工进度
  kEventKey_PublisherVideoProgress = 5,//表示视频总体加工进度
  kEventKey_Program  = 6, //纹理输入源进程的事件
};

enum cpp_event_data
{
  kEventData_None = 0,
  kEventData_Start = 1,   // 表示kEventKey_Receiver收到第一个音/视频帧 或 kEventKey_Publisher成功发送的第一个音/视频帧
  kEventData_Finish = 2,  // 推/拉流结束并且没有失败，与kEventData_Failed互斥
  kEventData_Failed = 3,  // 推/拉流失败，与kEventData_Finish互斥
  kEventData_OnFrame = 4, // 当使用cpp_start_capture进行frame hook时，每一帧都会回调kEventKey_Receiver的kEventData_OnFrame
  kEventData_Exit = 5,  // 进程程序退出命令
};

enum cpp_event_log_level
{
  kLogLevel_Trace = 0,
  kLogLevel_Debug = 1,
  kLogLevel_Info = 2,
  kLogLevel_Warning = 3,
  kLogLevel_Error = 4,
  kLogLevel_Fatal = 5,
};

struct cpp_event
{
  uint64_t uid;
  const char* streamUrl;
  const char* streamUUID;//简化处理推一路流的流名，加工程序调用cpp_pipeline_add_dx_texture_jce等函数对应的streamUUID参数
  const char* streamUUIDS;//解析推多路流的流名(包含上面一路流)：{"cpp_stream_uuids":["67-20230822161513167-1099531783474-virtuallive-cloud-28060-metashell-primary-out-src","67-20230822161513167-1099531783474-virtuallive-cloud-28060-metashell-secondary-out-src"]}
  int32_t eventData;
  cpp_event_key eventKey;
};

struct cpp_custom_data_event
{
  uint64_t uid;
  const char* streamUrl;
  const char* streamUUID;//简化处理推一路流的流名，加工程序调用cpp_pipeline_add_dx_texture_jce等函数对应的streamUUID参数
  const char* streamUUIDS;//解析推多路流的流名(包含上面一路流)：{"cpp_stream_uuids":["67-20230822161513167-1099531783474-virtuallive-cloud-28060-metashell-primary-out-src","67-20230822161513167-1099531783474-virtuallive-cloud-28060-metashell-secondary-out-src"]}
  const char* customData;
};

enum cpp_stream_type
{
  kStreamType_Huya = 0, 
  kStreamType_Rtmp = 1,
  kStreamType_Hls = 2,
  kStreamType_FlvFile = 3,  //本地flv文件
  kStreamType_Mp4File = 4,  //本地mp4文件
  kStreamType_Dx = 7,
  kStreamType_Camera = 12,
  kStreamType_Max = 255
};

enum cpp_kCodecType
{
  kCodecType_NONE = 1000,

  kCodecType_VIDEO_MIN = 1100,
  kCodecType_H264 = kCodecType_VIDEO_MIN,
  kCodecType_H265,
  kCodecType_VP8,
  kCodecType_VP9,
  kCodecType_VIDEO_MAX,

  kCodecType_AUDIO_MIN = 1200,
  kCodecType_OGG_OPUS = kCodecType_AUDIO_MIN,
  kCodecType_AAC,
  kCodecType_MP3,
  kCodecType_VORBIS,
  kCodecType_AUDIO_MAX,

  kCodecType_AUDIO_OTHER = 1300,
  kCodecType_PNG,
  kCodecType_JPEG,
  kCodecType_BMP,
};

enum cpp_codec_methodtype
{
  kAUTO = 0,
  kFFMPEG = 1,
  kNVCODEC = 2,
};

enum cpp_pixel_format 
{
  kPixel_Format_BGR24 = 0,
  kPixel_Format_BGRA = 1,
  kPixel_Format_RGB24 = 2,
  kPixel_Format_RGBA = 3,
  // B10G10R10A2
  kPixel_Format_30RA = 4,
  // R10G10B10A2
  kPixel_Format_30BA = 5,
  kPixel_Format_NV12 = 6,
  kPixel_Format_NV21 = 7,
  kPixel_Format_YUV420 = 8,
  kPixel_Format_RAW = 9,  //裸数据透传
  kPixel_Format_Count,
  kPixel_Format_Max = 255
};

struct cpp_plugin_program_cfg
{
  const char* programName;
  int32_t sort;
  const char* instanceGUID;
};

enum cpp_channel_status
{
  kConnected = 0,
  kDisconnected = 1,
  kHandShake = 2,
};

struct cpp_flow_cfg
{
  cpp_plugin_program_cfg* videoProgramList; //视频处理插件
  int32_t videoProgramListCount;
  cpp_plugin_program_cfg* audioProgramList; //音频处理插件
  int32_t audioProgramListCount;
};

struct cpp_puller_cfg
{
  cpp_stream_type type;
  // 流的唯一标识，仅用于pipeline内部，用来唯一标识一条流.
  const char* streamUUID;
  // 流的地址，根据type的值标识不同的含义. e.g. 
  // type == cpp_stream_type::kStreamType_Huya时, streamUrl中的是huya流的名字
  // type == cpp_stream_type::kStreamType_FLV时, streamUrl中的是flv文件的路径
  const char* streamUrl;
  int64_t appid;
  int64_t lUid;
  const char* cascadeTo = "";
  cpp_flow_cfg flowStreamConfig;
  bool needCaptureStream;
  //{"start_args": {"client_type": "PC", "work_mode": "both"}}
  const char* customData;
  const char* pipelineId;

  int32_t mode; //kMediaMode_Both = 0, kMediaMode_AudioOnly = 1, kMediaMode_VideoOnly = 2
  int32_t pixelFormat; //kPixelFormat_RGBA = 3, kPixelFormat_BGRA = 1, kPixelFormat_RAW = 9
  int32_t videoDecodeMethod;//kAUTO = 0, kFFMPEG = 1, kNVCODEC = 2
};

struct cpp_audio_codec
{
  int32_t channels;   //2
  int32_t sampleRate; //44100
  int32_t kbps;       //192
  int32_t codecType;  //见cpp_kCodecType
};

struct cpp_video_codec
{
  int32_t width;  //1920
  int32_t height; //1080
  int32_t fps;    //24
  int32_t kbps;   //10 * 1000
  int32_t codecType;  //见cpp_kCodecType
};

enum cpp_codec_texture_type
{
  KNO_TEXTURE = 0,
  KDX11_TEXTURE = 1,
};

struct cpp_pusher_cfg
{
  cpp_stream_type type;
  const char* streamUUID;
  const char* streamUrl;
  int64_t appid;
  int64_t lUid;
  const char* cascadeFrom = "";    //从puller或者compositor流入
  const char* cascadeTo = "";      //type=ENCODER_ONLY时有效，流向publisher_only,用于多路流合并
  cpp_audio_codec audioCodec; //音频参数
  cpp_video_codec videoCodec; //视频参数
  bool needCaptureStream;
  //"start_args": {"client_type": "PC", "work_mode": "both"}
  const char* customData;
  const char* pipelineId;
  int32_t mode; //kMediaMode_Both = 0, kMediaMode_AudioOnly = 1, kMediaMode_VideoOnly = 2
  int32_t videoEncodeMethod; //kAUTO = 0, kFFMPEG = 1, kNVCODEC = 2
  int32_t encodeTextureType; //0编码像素 1编码纹理
};

struct cpp_layout_rect
{
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
};

struct cpp_compositor_layout_config
{
  const char* streamUUID;
  cpp_layout_rect srcRect;
  cpp_layout_rect dstRect;
  bool hidden;
  int32_t zIndex;
  bool useAlpha;
};

struct cpp_compositor_cfg
{
  const char* compositorName;
  int64_t lUid;
  const char* cascadeFrom;  //从puller流入
  const char* cascadeTo;    //流向pusher
  cpp_flow_cfg flowStreamConfig;
  int32_t canvasWidth;
  int32_t canvasHeight;
  cpp_compositor_layout_config* layoutConfig;
  int32_t layoutConfigCount;
  // MediaMode
  // kMediaMode_Both音视频都会混化, 此为默认值.
  // kMediaMode_Audio/Video只混化音/视频，传入视/音频会被丢弃.
  int32_t mode;
};

struct cpp_plguin_data
{
  const char* programName;
  const char* programObjId;

  const char* dataKey;
  const char* dataValue;
};

struct cpp_raw_video_frame
{
  int32_t frameID;
  int32_t width;
  int32_t height;
  // 将pixelData看作height行、width列的图片，lineSize表示一行有多少个字节.e.g.
  int32_t lineSize;
  // 每个像素的颜色格式PixelFormat.
  int32_t pixelFormat;//参照cpp_pixel_format
  const char* pixelData;
  int32_t fps;
  //以下回调yuv数据支持
  char* planeData[8];
  int32_t planeDataSize[8];
  int32_t planeLineSize[8];
};

struct cpp_pcm_info
{
  int32_t sampleRate;
  // 每个声道的layout: AudioChannelLayout
  int32_t channelLayout;
  // 每个channel上sample的个数
  int32_t sampleCount;
  // 每个sample的格式: AudioFormat.
  int32_t audioFormat;
};

struct cpp_audio_callback_data
{
  const char* streamUUID;
  const cpp_pcm_info* raw;
  const uint8_t* data;
  uint32_t dataSize; 
  int64_t pts;
  const uint8_t* customData;
  uint32_t customDataSize;
};

struct cpp_video_callback_data
{
  const char* streamUUID; 
  const cpp_raw_video_frame* raw;
  int64_t pts;
  const uint8_t* customData;
  uint32_t customDataSize;
  int64_t sharedTextureHandle;
};

#ifdef __cplusplus
}
#endif
