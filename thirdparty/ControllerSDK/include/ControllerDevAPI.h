#pragma once

#include "ControllerAPIExports.h"
#include "ControllerAPIDef.h"
#include <stdint.h>

#if defined(__cplusplus)
extern "C"
{
#endif
struct CPPLogOption
{
  // 日志的路径,强烈建议使用绝对路径. 
  // 如果为null，则使用${MediaPipelineControllerSDK.dll所在目录}/../logs/
  const char* logDir;
  // 是否向logDir添加日期, mmdd的形式, e.g. /var/my_log_dir/0606/
  uint32_t useLogDirAsIs;
};
/*
@brief 初始化日志，需要在cpp_context_init前调用。提供更精细的日志初始化控制。
@param option 日志初始化的选项,如果为null，全部使用默认值
@return 返回0表示初始化成功
**/
CPP_EXPORT int cpp_log_init(CPPLogOption option);
/*
@brief 在Windows,hook程序的present dx调用，将绘制的内容当作名为streamName的流的内容推送给media pipeline.
@param streamName: hook的内容当作流的名字
@return 0 hook成功
**/
CPP_EXPORT int cpp_start_capture(const char* streamName, int frameRate);
CPP_EXPORT int cpp_stop_capture();

/*
@brief 请求将虚拟形象绘制结果推到Media pipeline中.
@param jobId: 任务id
@param streamUUID: 此前使用cpp_pipeline_add_or_update_xxxx添加的输入流的唯一标识
@param newLinkStr: 新的linkstr,关于link str的作用fenghezhou1@huya.com
@return 0 表示发送请求成功
*/
CPP_EXPORT int cpp_pipeline_set_puller_linkstr(const char* jobId,
  const char* streamUUID, const char* newLinkStr);

CPP_EXPORT int cpp_begin_transaction(const char* jobId);
CPP_EXPORT int cpp_add_or_update_puller(const char* jobId, const cpp_puller_cfg* cfg);
CPP_EXPORT int cpp_add_or_update_pusher(const char* jobId, const cpp_pusher_cfg* cfg);
CPP_EXPORT int cpp_add_or_update_publisher_only(const char* jobId, const cpp_pusher_cfg* cfg);
CPP_EXPORT int cpp_add_or_update_compositor(const char* jobId, const cpp_compositor_cfg* cfg);
CPP_EXPORT int cpp_remove_puller(const char* jobId, const char* streamUUID);
CPP_EXPORT int cpp_remove_pusher(const char* jobId, const char* streamUUID);
CPP_EXPORT int cpp_remove_publisher_only(const char* jobId, const char* streamUUID);
CPP_EXPORT int cpp_remove_compositor(const char* jobId, const char* streamUUID);
CPP_EXPORT int cpp_commit_transaction(const char* jobId);
CPP_EXPORT int cpp_add_plugin_data(const char* jobId, const cpp_plguin_data* data);
CPP_EXPORT int cpp_reset_dx_stream_pts(const char* streamUUID);
CPP_EXPORT int cpp_reset_dx_stream_pts2(const char* jobId, const char* streamUUID);
CPP_EXPORT int cpp_pipeline_stop(const char* jobId);
// Deprecated 已废弃，不要使用
CPP_EXPORT int cpp_pipeline_add_pcm(const char* streamUUID, const cpp_pcm_info* info, const uint8_t* srcData, uint32_t srcDataSize,
                                    const int64_t* pts = nullptr);
CPP_EXPORT int cpp_pipeline_add_raw(const char* streamUUID, const cpp_raw_video_frame* video_frame, const int64_t* captureStartTickMs = nullptr,
                                    const int64_t* pts = nullptr);
CPP_EXPORT int cpp_context_init(int32_t thriftPort, const char* jobId, const char* logDir);
CPP_EXPORT int cpp_context_init2(int32_t thriftPort, const char* jobId, const char* logDir, int32_t thriftListenPort);
CPP_EXPORT int cpp_pipeline_add_dx_texture(const char* streamUUID, uint64_t dxTextureHandle, const int64_t* pts = nullptr,
                                           const int64_t* captureStartTickMs = nullptr, const int64_t* fps = nullptr);
CPP_EXPORT int cpp_pipeline_add_texture(const char* streamUUID, void* context, void* texturePtr, int textureType = 0, const int64_t* pts = nullptr,
                                        const int64_t* fps = nullptr);
// Deprecated cpp_pipeline_share_pcm已废弃，不要使用
CPP_EXPORT int cpp_pipeline_share_pcm(const char* streamUUID, const cpp_pcm_info* info, const uint8_t* srcData, uint32_t srcDataSize,
                                      const int64_t* pts = nullptr);
CPP_EXPORT int cpp_pipeline_share_raw(const char* streamUUID, const cpp_raw_video_frame* video_frame, const int64_t* captureStartTickMs = nullptr,
                                      const int64_t* pts = nullptr);
CPP_EXPORT int cpp_pipeline_add_pcm_jce(const char* streamUUID, const cpp_pcm_info* info, const uint8_t* srcData, uint32_t srcDataSize,
                                        const int64_t* pts = nullptr, const char* customData = nullptr, uint32_t customDataSize = 0);
CPP_EXPORT int cpp_pipeline_add_raw_jce(const char* streamUUID, const cpp_raw_video_frame* video_frame, const char* customData,
                                        uint32_t customDataSize, const int64_t* pts = nullptr);
CPP_EXPORT int cpp_remote_log(const char* text, int32_t len);
typedef void(__stdcall* PFN_STREAM_LOG_CB)(void* userData, const char* pipelineId, cpp_event_log_level level, const char* logStr);
CPP_EXPORT int cpp_set_log_cb(PFN_STREAM_LOG_CB cb, void* userData);
typedef void(__stdcall* PFN_STREAM_AUDIODATA_CB)(void* userData, const char* pipelineId, const char* streamUUID, const cpp_pcm_info* raw,
                                                 const uint8_t* data, uint32_t dataSize, int64_t pts);
CPP_EXPORT int cpp_set_audio_data_cb(PFN_STREAM_AUDIODATA_CB cb, void* userData);
typedef void(__stdcall* PFN_STREAM_VIDEODATA_CB)(void* userData, const char* pipelineId, const char* streamUUID, const cpp_raw_video_frame* raw,
                                                 int64_t pts);
CPP_EXPORT int cpp_set_video_data_cb(PFN_STREAM_VIDEODATA_CB cb, void* userData);
#if defined(__cplusplus)
}
#endif
