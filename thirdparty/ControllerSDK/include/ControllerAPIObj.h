#pragma once

#include <cstdint>

#include "iframework.h"
#include "ControllerAPI.h"

////////////////////////////////////////////////////////////////////////
//! IMediapipelineApi
class IMediapipelineApi: public IInterface
{
public:
  // @SeeAlso cpp_context_init
  virtual int initContext(int32_t thriftPort, const char* jobId, const char* logDir) = 0;

  // @SeeAlso cpp_context_uninit
  virtual void uninitContext() = 0;

  virtual int beginTransaction(const char* jobId) = 0;

  virtual int addOrUpdatePuller(const char* jobId, const cpp_puller_cfg* cfg) = 0;

  virtual int addOrUpdatePusher(const char* jobId, const cpp_pusher_cfg* cfg) = 0;

  virtual int addOrUpdateCompositor(const char* jobId, const cpp_compositor_cfg* cfg) = 0;

  virtual int removePuller(const char* jobId, const char* streamName) = 0;

  virtual int removePusher(const char* jobId, const char* streamName) = 0;

  virtual int removeCompositor(const char* jobId, const char* streamName) = 0;

  virtual int commitTransaction(const char* jobId) = 0;

  virtual int addPluginData(const char* jobId, const cpp_plguin_data* data) = 0;

  // @SeeAlso cpp_pipeline_start
  virtual int stopPipeline(const char* jobId) = 0;

  virtual int addDXTexture(const char* streamUUID, uint64_t dxTextureHandle,
    const int64_t* pts = NULL, const int64_t* captureStartTickMs = NULL) = 0;

  // @SeeAlso cpp_start_capture
  virtual int startCapture(const char* streamName, int frameRate) = 0;
  // @SeeAlso cpp_stop_capture
  virtual int stopCapture() = 0;

  virtual int setLogCB(PFN_STREAM_LOG_CB cb, void* userData) = 0;
  virtual int setEventCB(PFN_STREAM_EVENT_CB cb, void* userData) = 0;

  // @SeeAlso cpp_pipeline_add_pcm
  virtual int addPcm(const char* streamName, const cpp_pcm_info* info,
    const uint8_t* srcData, uint32_t srcDataSize, const int64_t* pts = nullptr) = 0;

  // @SeeAlso cpp_pipeline_add_raw
  virtual int addRaw(const char* streamName, const cpp_raw_video_frame* video_frame,
    const int64_t* captureStartTickMs = nullptr,
    const int64_t* pts = nullptr) = 0;

  // @SeeAlso cpp_pipeline_add_raw_jce
  virtual int addRawJce(const char* streamName,
    const cpp_raw_video_frame* video_frame, const char* jceTsInfo, uint32_t sz,
    const int64_t* pts = nullptr) = 0;

  // @SeeAlso cpp_pipeline_get_tick_ms
  virtual int64_t getTickMs() = 0;

  // @SeeAlso cpp_pipeline_set_puller_linkstr
  virtual int setPullerLinkstr(const char * jobId, const char * streamUUID,
    const char * newLinkStr) = 0;

  virtual int addOrUpdatePublisherOnly(const char* jobId, const cpp_pusher_cfg* cfg) = 0;
  virtual int removePublisherOnly(const char* jobId, const char* streamName) = 0;

  virtual int resetDxStreamPts(const char* streamName) = 0;

  virtual int resetDxStreamPts2(const char* jobId, const char* streamName) = 0;
  virtual int setCustomDataCB(PFN_STREAM_CUSTOMDATA_CB cb, void* userData) = 0;
  virtual int setAudioDataCB(PFN_STREAM_AUDIODATA_CB cb, void* userData) = 0;
  virtual int setVideoDataCB(PFN_STREAM_VIDEODATA_CB cb, void* userData) = 0;
};

//Export information
#if defined(_WIN64)
#define MODULE_MEDIAPIPELINECONTROLLERSDK			"MediaPipelineControllerSDK.dll"
#elif defined(_WIN32)
#define MODULE_MEDIAPIPELINECONTROLLERSDK			"MediaPipelineControllerSDK.dll"
#else
#define MODULE_MEDIAPIPELINECONTROLLERSDK			"MediaPipelineControllerSDK.so"
#endif

#define INSTANCEFUNC_MEDIAPIPELINECONTROLLERSDK	  "CreateInstance_MediaPipelineControllerSDK"

using PFN_CreateInstance_MediaPipelineControllerSDK = IMediapipelineApi * (__stdcall *)();
