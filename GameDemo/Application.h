#pragma once
#include <memory>
#include <string>
#include <list>
#include <map>
#include "spdlog/spdlog.h"
#include "spdlog/details/os.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/dup_filter_sink.h"
#include <nlohmann/json.hpp>
#include <atomic>
#if __cplusplus >= 201703L
#include <filesystem>
using fs = std::filesystem;
#else
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

struct cpp_raw_video_frame;
struct cpp_pcm_info;

class Application
{
public:
  Application();
  ~Application();

  int Init(int argc, char** argv);
  int UnInit();
  int Run();

  void OnRecvStreamUUIDS(const std::string& streamUUIDS);
  void OnRecvCustomData(const std::string& customData);
  void GameRenderLoop();

  // 打包业务消息, 绑定流名和主播端图层名
  nlohmann::json BuildUpdateAnchorLayer();
  // 打包业务消息, 透传消息至小程序(小程序对应的hyExt.exe.onGameMessage可监听获取到该消息)
  nlohmann::json BuildSendMessageToApplet(const std::string& msg);
  // 发送业务消息
  void SendBizMsg(const std::string& eventName, const nlohmann::json& event);
  // 用于解析业务通道消息
  int OnRecvChannelMsg(std::string jsonStr);

public:
  void CreateHostWindow();
  void CreateGameWindow(HWND parent_hwnd);
  void CreateControlWindow(HWND parent_hwnd);
  void GetWindowPixels();
  void DumpImage(int32_t width, int32_t height, const char* buf, int32_t bufSize);
  void SendEditCustomMsg();
  void UpdateRecvEditMsg(const std::string& msg);
  void UpdateRecvEdit(const std::string& msg);

private:
  uint32_t m_thrifPort {0};
  uint32_t m_thrifListenPort {0};
  std::string m_jobId;
  std::unique_ptr<std::thread> m_gameRenderThread;
  std::atomic<bool> m_exitFlag;
  std::vector<std::string> m_streamNames;//解析获取流名，如有多场景流则含"primary"代表主流名，流名用于cpp_pipeline_share_raw_jce等推送音视频数据接口参数，一条流对应主播端一个预览图层
  int32_t m_resolutionWidth = 1920;
  int32_t m_resolutionHeight = 1080;

  HWND m_game_hwnd;
  HWND m_host_hwnd;
  HWND m_send_edit_hwnd;
  HWND m_recv_edit_hwnd;
  int32_t m_wndClientWidth = 1280;
  int32_t m_wndClientHeight = 720;
  std::string m_videoBuf;
  std::mutex m_videoBufMutex;
  int32_t m_reqId = 1;
};

static std::shared_ptr<spdlog::logger> m_logger;