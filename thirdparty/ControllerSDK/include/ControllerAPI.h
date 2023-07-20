#pragma once

#ifndef WIN32 // or something like that...
#define __stdcall
#else
#include <windows.h>
#endif

#include "ControllerAPIExports.h"
#include "ControllerAPIDef.h"

#if defined(__cplusplus)
extern "C"
{
#endif
//////////////////////////////////////////////////////////////////////////
/*
简单的例子:
*/
//////////////////////////////////////////////////////////////////////////
/*
@brief 初始化context
@param thriftPort: 与Media pipeline ipc时的端口号。注：port 和 port + 1都会被占用, 只有在本地联调时有效，正式环境会忽略该值
@param jobId: 已经启动的pipeline任务的id，可以通过CPP_JOB_ID环境变量/--cpp_jobid命令行参数获取
@param logDir: 日志文件写出的目录, 如果为null，则使用"当前模块目录/../logs/"
@return 0 初始化成功
**/
//! cpp_context_init
CPP_EXPORT int cpp_context_init(int32_t thriftPort, const char* jobId, const char* logDir);
/*
@param thriftListenPort: 本地模式的监听端口
**/
CPP_EXPORT int cpp_context_init2(int32_t thriftPort, const char* jobId, const char* logDir, int32_t thriftListenPort);

/*
@param enableDumpCapture: 控制是否隐式启动dump捕获
**/
CPP_EXPORT int cpp_context_init3(int32_t thriftPort, const char* jobId, const char* logDir, int32_t thriftListenPort, bool enableDumpCapture);

/*
@brief 反初始化context
*/
CPP_EXPORT void cpp_context_uninit();


//////////////////////////////////////////////////////////////////////////
//! cpp_dump_start path is utf8
CPP_EXPORT int32_t cpp_dump_start(const char* dumpDir);

//! cpp_dump_stop
CPP_EXPORT int32_t cpp_dump_stop();

/*
@brief 远程日志，在cpp_context_init后调用。提供远程日志的能力,频率不超过5条/1s, 线程安全。
@param text 日志内容，为utf8字符集
@param len 日志长度，会按len做截断,len <= 1024
@return 返回0表示调用成功，其它失败
**/
CPP_EXPORT int cpp_remote_log(const char* text, int32_t len);

/*
@brief 请求将虚拟形象绘制结果推到Media pipeline中.
@param streamUUID: 此前使用cpp_pipeline_set_input添加的输入流的唯一标识
@param dxTextureHandle: 虚拟形象绘制结果纹理,需要做成共享纹理，否则没办法跨进程发送到Pipeline.
@return 0 表示发送请求成功
*/
CPP_EXPORT int cpp_pipeline_add_dx_texture(const char* streamUUID,
  uint64_t dxTextureHandle, const int64_t* pts = nullptr,
  const int64_t* captureStartTickMs = nullptr,
  const int64_t* fps = nullptr);

CPP_EXPORT int cpp_pipeline_add_dx_texture_jce(const char* streamUUID,
  uint64_t dxTextureHandle, const int64_t* pts = nullptr,
  const int64_t* captureStartTickMs = nullptr,
  const int64_t* fps = nullptr, const char* customData = nullptr, uint32_t customDataSize = 0);

/*
@brief 从共享纹理句柄中获取ID3D11Resource*或像素数据，在context的使用线程调用
@param userData: 业务上下文
@param sharedHandle: 共享纹理句柄
@param context: DX设备上下文,例:ID3D11DeviceContext*，将以此对象去校验共享纹理句柄可用性, 能提供context则contextTexture填nullptr
@param contextTexture: 如果提供不了context，context就传nullptr, 需提供纹理指针ID3D11Resource*类型，由sdk在查找ID3D11DeviceContext*
@param texturePtr: 若校验共享纹理句柄成功，返回ID3D11Resource*, 则videoData->pixelData == nullptr
@param videoData: 若校验共享纹理句柄失败，则在此结构体videoData->pixelData返回数据, 则texturePtr == nullptr
@return 0 表示成功
*/
typedef void(__stdcall* PFN_DUMP_SHAREDTEXTURE_CB)(void* userData, void* texturePtr, cpp_raw_video_frame* videoData);
CPP_EXPORT int cpp_pipeline_dump_sharedtexture(PFN_DUMP_SHAREDTEXTURE_CB cb, void* userData, int64_t sharedHandle, void* context, void* contextTexture);

/*
@brief 接收纹理指针，发送给pipeline
@param streamUUID: 推送流名称标识
@param context: 纹理的上下文,例:ID3D11DeviceContext*, GLContext
@param texturePtr: 纹理指针,例:ID3D11Resource*, GLTexture
@param textureType: 纹理类型, 0:DX11, 1:DX9 2:GL 当前仅支持DX11
@return 0 表示发送请求成功
*/
CPP_EXPORT int cpp_pipeline_add_texture(const char* streamUUID, void* context, void* texturePtr, 
  int textureType = 0, const int64_t* pts = nullptr, const int64_t* fps = nullptr);

CPP_EXPORT int cpp_pipeline_add_texture_jce(const char* streamUUID, void* context, void* texturePtr,
  int textureType = 0, const int64_t* pts = nullptr, const int64_t* fps = nullptr, const char* customData = nullptr, uint32_t customDataSize = 0);

//Deprecated cpp_pipeline_add_pcm已废弃，不要使用
CPP_EXPORT int cpp_pipeline_add_pcm(const char* streamUUID, 
  const cpp_pcm_info* info, const uint8_t* srcData, uint32_t srcDataSize,
  const int64_t* pts = nullptr);

CPP_EXPORT int cpp_pipeline_add_pcm_jce(const char* streamUUID,
  const cpp_pcm_info* info, const uint8_t* srcData, uint32_t srcDataSize,
  const int64_t* pts = nullptr, const char* customData = nullptr, uint32_t customDataSize = 0);

// Deprecated cpp_pipeline_add_raw已废弃，不要使用
CPP_EXPORT int cpp_pipeline_add_raw(const char* streamUUID,
  const cpp_raw_video_frame* video_frame,
  const int64_t* captureStartTickMs = nullptr,
  const int64_t* pts = nullptr);

/*
@brief 将一个raw视频帧发送给pipeline
@param streamUUID: 此前使用cpp_pipeline_set_input添加的输入流的唯一标识
@pts 这一个帧的pts，如果pts == nullptr，由cpp_pipeline_add_raw生成.
*/
CPP_EXPORT int cpp_pipeline_add_raw_jce(const char* streamUUID,
  const cpp_raw_video_frame* video_frame, const char* customData, 
  uint32_t customDataSize, const int64_t* pts = nullptr);

// Deprecated cpp_pipeline_share_pcm已废弃，不要使用
CPP_EXPORT int cpp_pipeline_share_pcm(const char* streamUUID,
  const cpp_pcm_info* info, const uint8_t* srcData, uint32_t srcDataSize,
  const int64_t* pts = nullptr);

CPP_EXPORT int cpp_pipeline_share_pcm_jce(const char* streamUUID,
  const cpp_pcm_info* info, const uint8_t* srcData, uint32_t srcDataSize,
  const int64_t* pts = nullptr, const char* customData = nullptr, uint32_t customDataSize = 0);

/*
@brief 将一个raw视频帧发送给pipeline
@param streamUUID: 此前使用cpp_pipeline_set_input添加的输入流的唯一标识
@pts 这一个帧的pts，如果pts == nullptr，由cpp_pipeline_add_raw生成.
 **/
CPP_EXPORT int cpp_pipeline_share_raw(const char* streamUUID,
  const cpp_raw_video_frame* video_frame,
  const int64_t* captureStartTickMs = nullptr,
  const int64_t* pts = nullptr);

/*
@customData 自定义结构化数据.
 **/
CPP_EXPORT int cpp_pipeline_share_raw_jce(const char* streamUUID,
  const cpp_raw_video_frame* video_frame, const char* customData,
  uint32_t customDataSize, const int64_t* pts = nullptr);

// 获得统计上报的时间戳,虎牙控制网络对这个时间戳有所要求
CPP_EXPORT int64_t cpp_pipeline_get_tick_ms();

//日志回调，业务想接管日志的时候设置，回调函数需要保证线程安全
typedef void(__stdcall* PFN_STREAM_LOG_CB)(void* userData, const char* pipelineId, cpp_event_log_level level, const char* logStr);
CPP_EXPORT int cpp_set_log_cb(PFN_STREAM_LOG_CB cb, void* userData);

//流相关的事件，eventData的定义见cpp_event_data，回调函数需要保证线程安全
typedef void(__stdcall* PFN_STREAM_EVENT_CB)(void* userData, const char* pipelineId, const cpp_event* e);
CPP_EXPORT int cpp_set_event_cb(PFN_STREAM_EVENT_CB cb, void* userData);

typedef void(__stdcall* PFN_STREAM_CUSTOMDATA_CB)(void* userData, const char* pipelineId, const cpp_custom_data_event* e);
CPP_EXPORT int cpp_set_custom_data_cb(PFN_STREAM_CUSTOMDATA_CB cb, void* userData);

// Deprecated cpp_set_audio_data_cb已废弃，不要使用
typedef void(__stdcall* PFN_STREAM_AUDIODATA_CB)(void* userData, const char* pipelineId, const char* streamUUID, const cpp_pcm_info* raw, const uint8_t* data, uint32_t dataSize, int64_t pts);
CPP_EXPORT int cpp_set_audio_data_cb(PFN_STREAM_AUDIODATA_CB cb, void* userData);

//Deprecated cpp_set_video_data_cb已废弃，不要使用
typedef void(__stdcall* PFN_STREAM_VIDEODATA_CB)(void* userData, const char* pipelineId, const char* streamUUID, const cpp_raw_video_frame* raw, int64_t pts);
CPP_EXPORT int cpp_set_video_data_cb(PFN_STREAM_VIDEODATA_CB cb, void* userData);

typedef void(__stdcall* PFN_STREAM_AUDIODATA_CB2)(void* userData, const char* pipelineId, cpp_audio_callback_data* audioData);
CPP_EXPORT int cpp_set_audio_data_cb2(PFN_STREAM_AUDIODATA_CB2 cb, void* userData);

/*
@brief 设置视频帧回调,包含纹理和数据两种.
@param videoData: sharedTextureHandle等于0，则使用cpp_raw_video_frame对象数据;sharedTextureHandle不等于0，则cpp_raw_video_frame指针为nullptr，使用共享纹理句柄
@param userData: 调用方上下文
@return 0 表示成功
*/
typedef void(__stdcall* PFN_STREAM_VIDEODATA_CB2)(void* userData, const char* pipelineId, cpp_video_callback_data* videoData);
CPP_EXPORT int cpp_set_video_data_cb2(PFN_STREAM_VIDEODATA_CB2 cb, void* userData);

/*
@发送信息给thriftservice
 **/
CPP_EXPORT int cpp_call_channel_msg(const char* sendBuf, uint32_t sendBufLen);

/*
@brief 设置接收业务数据的回调接口.
@param msg: 业务数据，需要解析
@param userData: 调用方上下文
@return 0 表示成功
*/
typedef void(__stdcall* PFN_CHANNEL_MSG_CB)(void *userData, const char* msg, uint32_t len);
CPP_EXPORT int cpp_set_channel_msg_cb(PFN_CHANNEL_MSG_CB cb, void* userData);

/*
@brief 独立使用编解码、推拉流能力.
@pipelineId代表分组,保持调用分组一致即可, 业务如果不想关心就固定填"cppsdk-use-pull-pusher", 必须与cpp_puller_cfg/cpp_pusher_cfg的pipelineId保持一致
@增加更新删除拉流对象, cfgs代表全量, 例cfgs = { stream1, stream2}, 调用 { stream1 }, 则会自动查找删除stream2
@cfgs代表全量, 移除所有对象, 例:cpp_add_or_update_puller_ex("id", nullptr);
*/
CPP_EXPORT int cpp_add_or_update_puller_ex(const char* pipelineId, const cpp_puller_cfg* cfgs, int32_t len);
CPP_EXPORT int cpp_add_or_update_pusher_ex(const char* pipelineId, const cpp_pusher_cfg* cfgs, int32_t len);
CPP_EXPORT int cpp_push_video(const cpp_video_callback_data* videoData);
CPP_EXPORT int cpp_push_audio(const cpp_audio_callback_data* audioData);
#if __cplusplus
}
#endif
