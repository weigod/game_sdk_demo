#pragma once
#include "ControllerAPIExports.h"
#include "ControllerAPIDef.h"

#if defined(__cplusplus)
extern "C"
{
#endif
//////////////////////////////////////////////////////////////////////////

/**  可以通过环境变量来获取端口
   * 环境变量样例: CPP_APPID=67 CPP_PORT=4760
   * CPP_JOB_ID=local_67-20230718194817278-1099531783474-17244
   * CPP_STREAM_UUID=67-20230718194730305-1099531783474-fish-local-32080-metashell-primary-out
   * CPP_STREAM_UUIDS=eyJjcHBfc3RyZWFtX3V1aWRzIjpbIjY3LTIwMjMwNzE4MTk0NzMwMzA1LTEwOTk1MzE3ODM0NzQtZmlzaC1sb2NhbC0zMjA4MC1tZXRhc2hlbGwtcHJpbWFyeS1vdXQiXX0=
   * CPP_STREAM_UID=1099531783474 CPP_PROGRAM_NAME=virtual_metaroom
   * CPP_PID=17244
   * LOCAL_LOG_DIR="C:\Users\Administrator\AppData\Roaming\huyaPcPresenter\presenter\log\0801\153902_478\metashell\59ku7pqm\2023-08-01 15-39-23-219"
   * 字符编码为utf8
**
@brief 初始化CPPSDK
@param thriftPort: ipc的端口号, 对应环境变量CPP_PORT
@param jobId: 任务id，对应环境变量CPP_JOB_ID
@param logDir: 日志文件写出的目录, 如果为null，则使用"当前模块目录/../logs/", //对应环境变量LOCAL_LOG_DIR
@param thriftListenPort: 反向监听端口，除了调试模式外其他场景都填0
@param enableDumpCapture: true: 启用sdk的dump捕获能力，false: 不启用
@return 0 初始化成功
**/
CPP_EXPORT int cpp_context_init3(int32_t thriftPort, const char* jobId, const char* logDir, int32_t thriftListenPort, bool enableDumpCapture);

/**  
   * sdk自动从获取了环境变量CPP_PORT，CPP_JOB_ID，LOCAL_LOG_DIR
   * 等同于调了cpp_context_init3(CPP_PORT, CPP_JOB_ID, LOCAL_LOG_DIR, 0, false)
**
@return 0 初始化成功
**/
CPP_EXPORT int cpp_context_init_ex();

/*
@brief 反初始化媒体控制SDK
*/
CPP_EXPORT void cpp_context_uninit();

/**
 * @brief 启用SDK的崩溃捕获,cpp_context_init3 enableDumpCapture: true
 * @dumpDir 崩溃产生的路径，要求utf8
 */
CPP_EXPORT int32_t cpp_dump_start(const char* dumpDir);

/**
 * @brief 停用SDK的崩溃捕获
 */
CPP_EXPORT int32_t cpp_dump_stop();

/*
@brief 发送共享纹理(已兼容多显卡实现)
@param streamUUID: 流名
@param dxTextureHandle: DX11共享纹理
@param customData: 用于发送帧绑定的数据，等同于SEI
@return 0 表示发送请求成功
*/
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
@brief 发送纹理指针，内部会转成共享纹理或数据发送(已兼容多显卡实现)
@param streamUUID: 推送流名称标识
@param context: 纹理的上下文,例:ID3D11DeviceContext*, GLContext
@param texturePtr: 纹理指针,例:ID3D11Resource*, GLTexture
@param textureType: 纹理类型, 0:DX11, 1:DX9 2:GL 当前仅支持DX11
@param customData: 用于发送帧绑定的数据，等同于SEI
@return 0 表示发送请求成功
*/
CPP_EXPORT int cpp_pipeline_add_texture_jce(const char* streamUUID, void* context, void* texturePtr,
  int textureType = 0, const int64_t* pts = nullptr, const int64_t* fps = nullptr, const char* customData = nullptr, uint32_t customDataSize = 0);

/*
@brief 共享内存发送音频帧
@param streamUUID: 流的唯一标识
@pts 这一个帧的pts，如果pts == nullptr，则自动生成.
@param customData: 用于发送帧绑定的数据，等同于SEI
*/
CPP_EXPORT int cpp_pipeline_share_pcm_jce(const char* streamUUID,
  const cpp_pcm_info* info, const uint8_t* srcData, uint32_t srcDataSize,
  const int64_t* pts = nullptr, const char* customData = nullptr, uint32_t customDataSize = 0);

/*
@brief 共享内存发送视频帧
@param streamUUID: 流的唯一标识
@pts 这一个帧的pts，如果pts == nullptr，则自动生成.
@param customData: 用于发送帧绑定的数据，等同于SEI
 **/
CPP_EXPORT int cpp_pipeline_share_raw_jce(const char* streamUUID,
  const cpp_raw_video_frame* video_frame, const char* customData,
  uint32_t customDataSize, const int64_t* pts = nullptr);

/**
 * @brief 统计上报的时间戳,虎牙控制网络对这个时间戳有所要求
 */
CPP_EXPORT int64_t cpp_pipeline_get_tick_ms();

/**
 * @brief 流相关的事件回调，eventData的定义见cpp_event_data，回调函数需要保证线程安全，不理解可以不关心
 */
typedef void(__stdcall* PFN_STREAM_EVENT_CB)(void* userData, const char* pipelineId, const cpp_event* e);
CPP_EXPORT int cpp_set_event_cb(PFN_STREAM_EVENT_CB cb, void* userData);

/**
 * @brief customdata事件回调，用于透传自定义信息，信息丰富，要求处理，通常为json字符串(端云都调此接口)
 */
typedef void(__stdcall* PFN_STREAM_CUSTOMDATA_CB)(void* userData, const char* pipelineId, const cpp_custom_data_event* e);
CPP_EXPORT int cpp_set_custom_data_cb(PFN_STREAM_CUSTOMDATA_CB cb, void* userData);

/**
 * @brief 音频帧回调回调，用于接受音频帧的处理
 * @audioData 音频帧描述结构体
 * @param userData: 调用方上下文
 */
typedef void(__stdcall* PFN_STREAM_AUDIODATA_CB2)(void* userData, const char* pipelineId, cpp_audio_callback_data* audioData);
CPP_EXPORT int cpp_set_audio_data_cb2(PFN_STREAM_AUDIODATA_CB2 cb, void* userData);

/*
@brief 视频帧回调,包含纹理和数据两种.
@param videoData: sharedTextureHandle等于0，则使用cpp_raw_video_frame对象数据;sharedTextureHandle不等于0，则cpp_raw_video_frame指针为nullptr，使用共享纹理句柄
@param userData: 调用方上下文
@return 0 表示成功
*/
typedef void(__stdcall* PFN_STREAM_VIDEODATA_CB2)(void* userData, const char* pipelineId, cpp_video_callback_data* videoData);
CPP_EXPORT int cpp_set_video_data_cb2(PFN_STREAM_VIDEODATA_CB2 cb, void* userData);

/**
 * @brief 发送业务数据(端云都调此接口)
 * @sendBuf 业务数据
 * @return 0 表示成功
 */
CPP_EXPORT int cpp_call_channel_msg(const char* sendBuf, uint32_t sendBufLen);

/*
@brief 设置接收业务数据的回调接口.
@param msg: 业务数据，需要解析，通常为json字符串，由业务定义
@param userData: 调用方上下文
@return 0 表示成功
*/
typedef void(__stdcall* PFN_CHANNEL_MSG_CB)(void *userData, const char* msg, uint32_t len);
CPP_EXPORT int cpp_set_channel_msg_cb(PFN_CHANNEL_MSG_CB cb, void* userData);

/*
@brief 设置通道状态回调
@param status: 通道状态kConnected = 0, kDisconnected = 1, kHandShake = 2，需收到kHandShake事件，代表通道建立成功，方可发送业务数据(端云通用)
@param userData: 调用方上下文
@return 0 表示成功
*/
typedef void(__stdcall* PFN_CHANNEL_STATUS)(void* userData, cpp_channel_status status);
CPP_EXPORT int cpp_set_channel_status_cb(PFN_CHANNEL_STATUS cb, void* userData);

/*
@brief 扩展功能 => 独立使用编解码、推拉流能力.
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
