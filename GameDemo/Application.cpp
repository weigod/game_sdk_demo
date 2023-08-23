#include "Application.h"

#include <thread>
#include "version.h"
#include "ControllerAPI.h"
#include "EnvUtil.h"
#include "base64.h"
#include <iostream>
#include <sstream>
#include<iomanip>
#ifdef WIN32
#include <timeapi.h>
#include "DxTextureAdapter.h"
#endif
#include "winuser.h"
#include "libyuv.h"
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "string_utils.h"

std::string GetLogSuffix()
{
  std::ostringstream s;
  std::time_t t = std::time(NULL);
  std::tm* ft = std::localtime(&t);

  s << std::setfill('0')
    << std::setw(2) << ft->tm_mon + 1
    << std::setw(2) << ft->tm_mday
    << "_"
    << std::setw(2) << ft->tm_hour
    << std::setw(2) << ft->tm_min
    << std::setw(2) << ft->tm_sec
    << "-"
    << spdlog::details::os::pid()
    << ".log";

  return s.str();
}

#define SEND_MSG_BTN_ID 10001
#define WM_RECV_UPDATE_MSG (WM_USER + 100)

////////////////////////////////////////////////////////////////////////
Application::Application()
{
#ifdef WIN32
  timeBeginPeriod(1);
#endif
}

Application::~Application()
{
#ifdef WIN32
  timeEndPeriod(1);
#endif
  srand(static_cast<unsigned>(time(nullptr)));
}

int Application::Init(int argc, char** argv)
{
  /* 可以通过启动参数来获取端口
   * 启动参数样例: --cpp_appid=67 --cpp_port=4760 --cpp_listen_port=0
   * --cpp_jobid=local_67-20230718194817278-1099531783474-17244
   * --cpp_stream_uuid=67-20230718194730305-1099531783474-fish-local-32080-metashell-primary-out
   * --cpp_stream_uuids=eyJjcHBfc3RyZWFtX3V1aWRzIjpbIjY3LTIwMjMwNzE4MTk0NzMwMzA1LTEwOTk1MzE3ODM0NzQtZmlzaC1sb2NhbC0zMjA4MC1tZXRhc2hlbGwtcHJpbWFyeS1vdXQiXX0=
   * --cpp_stream_uid=1099531783474 --cpp_program_name=virtual_metaroom
   * --cpp_pid=17244
   * --local_log_dir="C:\Users\Administrator\AppData\Roaming\huyaPcPresenter\presenter\log\0801\153902_478\metashell\59ku7pqm\2023-08-01 15-39-23-219"
   * 字符编码为utf8
   */

  // 或通过环境变量获取端口
  std::string cppPort = cpp::GetEnvVar("CPP_PORT");//对应启动参数--cpp_port
  if (cppPort.empty())
  {
    // 如果环境变量获取的端口为空，则赋为0
    cppPort = "0";
  }
  m_thrifPort = std::stoll(cppPort);

  std::string cppListenPort = cpp::GetEnvVar("CPP_LISTEN_PORT");  // 对应启动参数--cpp_listen_port 通常为0
  if (cppListenPort.empty()) 
  {
    // 如果环境变量获取的端口为空，则赋为0
    cppListenPort = "0";
  }
  m_thrifListenPort = std::stoll(cppListenPort);

  m_jobId = cpp::GetEnvVar("CPP_JOB_ID");//对应启动参数--cpp_jobid
  std::string logDir = cpp::GetEnvVar("LOCAL_LOG_DIR");//对应启动参数--local_log_dir
  fs::path logPath;
  if (!logDir.empty()) {
    logPath = fs::u8path(logDir);
  } else {
    fs::path exePath = argv[0];
    logPath = exePath.parent_path();
    logPath = logPath / "logs";
  }

  // 初始化游戏日志,日志目录必须设为启动参数或环境变量所指定的目录local_log_dir
  fs::path logFile(logPath);
  std::error_code errCode;
  if (!fs::exists(logPath) && !fs::create_directories(logPath, errCode))
  {
    Sleep(2000);
    return -1;
  }
  std::vector<spdlog::sink_ptr> sinks;
  logFile = logFile / ("GameDemo_" + GetLogSuffix());
  auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFile.u8string().c_str(), 1024 * 1024 * 10, 3);
  sinks.emplace_back(rotating_sink);
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::info);
  sinks.emplace_back(console_sink);

  m_logger = std::make_shared<spdlog::logger>("GameDemo", sinks.begin(), sinks.end());
  m_logger->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e][%l][%t] %v%$");
  spdlog::flush_every(std::chrono::seconds(1));
  m_logger->flush_on(spdlog::level::info);
  spdlog::register_logger(m_logger);

  m_logger->info("GameDemo start thrifPort:{}, thrifListenPort:{}, jobId:{}, local_log_dir:{}", m_thrifPort, m_thrifListenPort, m_jobId, logPath.u8string());

  // 初始化SDK, SDK的日志目录也必须设为启动参数或环境变量所指定的目录local_log_dir
  // 注意：enableDumpCapture填true会启用sdk的崩溃捕获能力，会让游戏自带的崩溃捕获失效
  auto err = cpp_context_init3(m_thrifPort, m_jobId.c_str(), logPath.u8string().c_str(), m_thrifListenPort, false);
  if (err != 0)
  {
    m_logger->error("cpp_context_init err: {}", err);
    return err;
  }

  // 设置customdata回调
  cpp_set_custom_data_cb([](void* userData, const char* pipelineId, const cpp_custom_data_event* e){
    m_logger->info("custom_data_cb pipelineId: {}, custom_data_event customData: {}, streamUrl: {}, streamUUIDS: {}", 
      pipelineId, e->customData, e->streamUrl, e->streamUUIDS);
    Application *contxt = static_cast<Application *>(userData);
    //解析streamUUIDS获取推流的目标流名，多路流就对应多个流名
    contxt->OnRecvStreamUUIDS(e->streamUUIDS);
    contxt->OnRecvCustomData(e->customData);
  }, this);

  //注册接收业务通道状态回调//status： kConnected = 0, kDisconnected = 1, kHandShake = 2
  //收到kHandShake = 2表示通道握手成功，可开始发送通道消息
//   cpp_set_channel_status_cb([](void* userData, cpp_channel_status status) { 
//     m_logger->info("recv channel status: {}", status);
//   }, this);
  //注册接收业务通道消息回调，对应cpp_call_channel_msg用于发送业务通道消息
  cpp_set_channel_msg_cb([](void* userData, const char* msg, uint32_t len) { 
    Application* context = (Application*)userData;
    std::string message(msg, len);
    m_logger->info("recv channel msg: {}", message);
    context->OnRecvChannelMsg(message);
  }, this);

  CreateHostWindow();
  return 0;
}

int Application::Run()
{
  // 主消息循环:
  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    if (!TranslateAccelerator(msg.hwnd, NULL, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return (int)msg.wParam;
}

void Application::OnRecvStreamUUIDS(const std::string& streamUUIDS) 
{
  try {
    auto jsonObj = nlohmann::json::parse(streamUUIDS);
    if (jsonObj.contains("cpp_stream_uuids")) {
      auto uuidsList = jsonObj["cpp_stream_uuids"];
      if (uuidsList.is_array()) {
        for (auto& v : uuidsList) {
          m_streamNames.push_back(v.get<std::string>());
          m_logger->info("OnParseStreamUUIDS streamname: {}", v.get<std::string>());
        }
      }
    }
  } catch (const std::exception& e) {
    m_logger->error("parse customData failed with error:: {}", e.what());
  }

  if (!m_gameRenderThread) {
    SendBizMsg("UpdateAnchorLayer", BuildUpdateAnchorLayer());
    m_gameRenderThread = std::make_unique<std::thread>(&Application::GameRenderLoop, this);
  }
}

void Application::OnRecvCustomData(const std::string& customData)
{
  m_logger->info("OnRecvCustomData start. customData: {}", customData);
}

void Application::GameRenderLoop() 
{ 
  //模拟游戏30fps发送图像数据
  while (!m_exitFlag.load())
  {
    std::string videoBuf {0};
    {
      std::lock_guard<std::mutex> l(m_videoBufMutex);
      videoBuf.swap(m_videoBuf);
    }

    if (videoBuf.size() < m_wndClientWidth * m_wndClientHeight * 4) 
    {
      continue;
    }

    //模拟创建一个游戏纹理
    static std::shared_ptr<DxTextureAdapter> nativeAdapter = nullptr;
    static ID3D11Texture2D* pTexture = nullptr;//
    if (!nativeAdapter)
    {
      nativeAdapter.reset(new DxTextureAdapter);
      nativeAdapter->Init();
      HRESULT hRet = 0;
      D3D11_TEXTURE2D_DESC desc = {};
      desc.Width = m_wndClientWidth;
      desc.Height = m_wndClientHeight;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = (DXGI_FORMAT)DXGI_FORMAT_R8G8B8A8_UNORM;

      desc.SampleDesc.Count = 1;
      desc.Usage = D3D11_USAGE_STAGING;
      desc.MiscFlags = 0;
      desc.BindFlags = 0;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      HRESULT hr = nativeAdapter->GetDevice()->CreateTexture2D(&desc, NULL, &pTexture);
    }

    //发送游戏纹理数据，支持下面三种方式
    //1、支持发送ID3D11Resource* pTexture
    //cpp_pipeline_add_texture_jce(m_streamName.c_str(), nullptr, pTexture, 0, nullptr, nullptr, nullptr, 0);
    //2、支持发送裸数据
    cpp_raw_video_frame video_frame;
    video_frame.width = m_wndClientWidth;
    video_frame.height = m_wndClientHeight;
    video_frame.lineSize = m_wndClientWidth * 4;
    video_frame.pixelData = videoBuf.data();
    video_frame.pixelFormat = kPixel_Format_RGBA;
    video_frame.fps = 30;
    cpp_pipeline_share_raw_jce(m_streamNames[0].c_str(), &video_frame, nullptr, 0, nullptr);
    //3、发送共享纹理句柄
    //cpp_pipeline_add_dx_texture_jce(m_streamName.c_str(), sharedHandle);

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }
}

/* 通用业务协议头 { "eventName": "UpdateAnchorLayer", "reqId": "100", "message": {} }
 *
 * eventName String 请求时函数名，如UpdateAnchorLayer
 * message String 请求消息(json对象字符串)
 * reqId String 请求时的对应reqId(建议按照递增传参，即游戏内部管理计数依次递增+1即可，如：1,2,3...))
 * message Object 实际的请求数据内容(json对象，不同请求时对应的json应答结构，作为以上各个接口的入参参数json对象)
 */

/*
 * UpdateAnchorLayer 为二级协议，对应上面message字段， 功能:
 * 更新加工图层信息 传入的Object参数中，其中格式如下： layerList Array
 * 加工后图层列表 x Number 相对画布左上角位置x坐标(像素为单位) y Number
 * 相对画布左上角位置y坐标(像素为单位) width Number
 * 相对画布的显示宽度(像素为单位，0时表示使用流内容的宽度) height Number
 * 相对画布的显示高度(像素为单位，0时表示使用流内容的高度) stream_name String
 * 加工流名称(此值一定要与写加工后流数据的流名称一致，唯一, is_primary Boolean
 * 是否为主流(此流将会替换加工摄像头画面或创建替换主加工画面，唯一，其他为false的图层为次图层)
 * opt_layer_name String
 * 可选的图层名称(此值用于显示到主播端图层列表里显示的名称，可选(为空字符串时，比如小程序名称为:"无敌战车"，则默认主图层名称为:"无敌战车")
 */
nlohmann::json Application::BuildUpdateAnchorLayer() {
  nlohmann::json innerEvent;
  if (m_streamNames.size() <= 0) {
    m_logger->info("BuildUpdateAnchorLayer m_streamNames is NULL");
    return innerEvent;
  }
  innerEvent["layerList"] = nlohmann::json::array();
  nlohmann::json item = {
      {"x", 0},
      {"y", 0},
      {"width", 1280},
      {"height", 720}, 
      {"stream_name", m_streamNames[0]},
      /*{"opt_layer_name", u8"游戏Demo"},*///名字跟小程序名一致
      {"is_primary", true}};
  innerEvent["layerList"].push_back(item);
  return innerEvent;
}

nlohmann::json Application::BuildSendMessageToApplet(const std::string& msg) {
  nlohmann::json innerEvent;
  innerEvent["msg"] = msg;
  return innerEvent;
}

void Application::SendBizMsg(const std::string& eventName, const nlohmann::json& event) {
  nlohmann::json bizMsg = {{"eventName", eventName}, {"reqId", std::to_string(m_reqId++)}, {"message", event}};
  std::string jsonStr = bizMsg.dump(-1, ' ', false, nlohmann::detail::error_handler_t::ignore);
  m_logger->info("SendBizMsg jsonStr: {}", jsonStr);
  cpp_call_channel_msg(jsonStr.data(), jsonStr.size());
}

int Application::UnInit()
{
  m_exitFlag.store(true);
  cpp_context_uninit();
  return 0;
}

/*
 * 解析业务通道消息包
 * 示例： { "eventName": "UpdateAnchorLayer_Callback", "reqId": "100", "res": 0, "message": {} }
 * 应答或通告数据为json对象形式的字符串，含义如下：
 * eventName String 对应请求时的回调函数名，比如请求UpdateAnchorLayer时，该应答值将为UpdateAnchorLayer_Callback(即_Callback后缀)
 * reqId String 对应请求时reqId
 * res Number 返回错误码，0成功，非0失败(-1:参数错误 -2:未准备就绪 -3:其他错误)
 * message Object 实际的数据内容(json对象，不同请求时或通告事件时对应的json应答结构)(当res为0时，message值才有效)
 * 注：当event为通告事件时(即以On开头的事件)，如OnAppletMessage，该event将不带_Callback后缀且其值为OnAppletMessage，此外reqId为0，res为0
 */
int Application::OnRecvChannelMsg(std::string jsonStr)
{
  if (jsonStr.empty())
  {
    return -1;
  }
  std::string reqId = "", eventName = "";
  nlohmann::json messageObj;
  int res = 0;
  try
  {
    auto jsonObj = nlohmann::json::parse(jsonStr);
    if (jsonObj.contains("message")) {
      messageObj = jsonObj["message"];
    }
    if (jsonObj.contains("eventName")) {
      eventName = jsonObj["eventName"].get<std::string>();
    }
    if (jsonObj.contains("reqId")) {
      auto reqId = jsonObj["reqId"].get<std::string>();
    }
    if (jsonObj.contains("res")) {
      auto res = jsonObj["res"].get<int32_t>();
    }
  }
  catch (const std::exception& e)
  {
    m_logger->error("parse GetHostCameraLayerInfo data failed with error:: {}, jsonStr: {} ", e.what(), jsonStr);
    return -2;
  }

  if (eventName == "OnAppletMessage") //小程序发过来的消息
  {
    if (messageObj.contains("msg")) {
      auto msg = messageObj["msg"].get<std::string>();
      m_logger->info("OnAppletMessage msg: {}", msg);
      UpdateRecvEditMsg(msg);
    }
  } 
  else if (eventName == "OnAnchorCanvasChange") //主播端的画布改变的消息
  {
    bool sceneChanged = false;
    if (messageObj.contains("sceneChanged")) {
      sceneChanged = messageObj["sceneChanged"].get<bool>();
    }

    int32_t layoutType = 0;
    if (messageObj.contains("layoutType")) {
      layoutType = messageObj["layoutType"].get<int32_t>();
    }
    if (messageObj.contains("width")) {
      m_resolutionWidth = messageObj["width"].get<int32_t>(); //开播预览分辨率(画布)分辨率改变
    }
    if (messageObj.contains("height")) {
      m_resolutionHeight = messageObj["height"].get<int32_t>(); //开播预览分辨率(画布)分辨率改变
    }
    m_logger->info("OnAnchorCanvasChange, scene changed: {}, layout type: {}, width: {}, height: {}",
      sceneChanged, layoutType, m_resolutionWidth, m_resolutionHeight);
  }
  else if (eventName == "OnOperateLayer") //主播端的图层操作消息
  {
    int32_t type = 0;
    if (messageObj.contains("type")) {
      type = messageObj["type"].get<int32_t>();
    }

    std::string name;
    if (messageObj.contains("name")) {
      name = messageObj["name"].get<std::string>();
    }
    m_logger->info("OnOperateLayer, type: {}, name: {}", type, name);
  }
  else if (eventName == "UpdateAnchorLayer_Callback")  // UpdateAnchorLayer请求的应答消息
  {
    if (messageObj.contains("msg")) {
      auto msg = messageObj["msg"].get<std::string>();
      m_logger->info("UpdateAnchorLayer_Callback msg: {}", msg);
    }
  }
  else if (eventName == "SendMessageToApplet_Callback")  // SendMessageToApplet请求的应答消息
  {
    if (messageObj.contains("msg")) {
      auto msg = messageObj["msg"].get<std::string>();
      m_logger->info("SendMessageToApplet_Callback msg: {}", msg);
    }
  }
  return 0;
}

static UINT_PTR timerId;
LRESULT CALLBACK HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_COMMAND: {
    if (LOWORD(wParam) == SEND_MSG_BTN_ID) {
      Application* context = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
      context->SendEditCustomMsg();
    }
  } break;
  case WM_RECV_UPDATE_MSG: {
    Application* context = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    std::string* msg_str = (std::string*)lParam;
    if (msg_str) {
      context->UpdateRecvEdit(*msg_str);
      delete msg_str;
    }
  } break;
  case WM_DESTROY: {
    Application* context = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    context->SendBizMsg("SendMessageToApplet", context->BuildSendMessageToApplet("GameExit"));
    KillTimer(hWnd, timerId);  // 销毁定时器
    PostQuitMessage(0);
  } break;
  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_CREATE: 
    {
      timerId = SetTimer(hWnd, 1, 10, nullptr);  // 设置定时器，每10毫秒触发一次
      LONG style = GetWindowLong(hWnd, GWL_STYLE);
      style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
      SetWindowLong(hWnd, GWL_STYLE, style);
    } break;
    case WM_TIMER:
      InvalidateRect(hWnd, nullptr, TRUE);  // 请求窗口重绘
      break;
    case WM_PAINT: {
      HDC hdc;
      PAINTSTRUCT ps;
      hdc = BeginPaint(hWnd, &ps);

      RECT clientRect;
      GetClientRect(hWnd, &clientRect);

      static double counter = 0;
      counter += 0.005;
      if (counter >= 360.0) counter -= 360.0;

      int r = (int)((sin(counter) + 1.0) * 127.5);
      int g = (int)((sin(counter + 120.0) + 1.0) * 127.5);
      int b = (int)((sin(counter + 240.0) + 1.0) * 127.5);

      HBRUSH hBrush = CreateSolidBrush(RGB(r, g, b));
      FillRect(hdc, &clientRect, hBrush);
      DeleteObject(hBrush);

      EndPaint(hWnd, &ps);
      //获取窗口像素
      Application* context = (Application*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
      context->GetWindowPixels();
    } break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

void Application::CreateGameWindow(HWND parent_hwnd) {
  WNDCLASSEXW wcex;
  wcex.cbSize = sizeof(WNDCLASSEXW);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = GetModuleHandleW(nullptr);
  wcex.hIcon = NULL;
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = NULL;  // 移除背景画刷，由WM_PAINT处理器负责绘制背景
  wcex.lpszMenuName = NULL;
  wcex.lpszClassName = L"Dummy_Unity_Class";
  wcex.hIconSm = NULL;

  RegisterClassExW(&wcex);

  int clientWidth = m_wndClientWidth;
  int clientHeight = m_wndClientHeight;
  DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

  RECT windowRect = {0, 0, clientWidth, clientHeight};
  AdjustWindowRect(&windowRect, style, FALSE);

  int windowWidth = windowRect.right - windowRect.left;
  int windowHeight = windowRect.bottom - windowRect.top;

  m_game_hwnd = CreateWindowW(L"Dummy_Unity_Class", L"Dummy_Unity", style,
                       CW_USEDEFAULT, 0, windowWidth, windowHeight, parent_hwnd,
                       nullptr, GetModuleHandleW(nullptr), nullptr);
  SetWindowLongPtr(m_game_hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

void Application::CreateHostWindow() {
  WNDCLASSEXW wcex;
  wcex.cbSize = sizeof(WNDCLASSEXW);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = HostWndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = GetModuleHandleW(nullptr);
  wcex.hIcon = NULL;
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = NULL;
  wcex.lpszClassName = L"Dummy_Host_Class";
  wcex.hIconSm = NULL;

  RegisterClassExW(&wcex);
  m_host_hwnd = CreateWindowW(L"Dummy_Host_Class", L"DummyGame", WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
    CW_USEDEFAULT, CW_USEDEFAULT, 1600, 780, nullptr,
    nullptr, GetModuleHandleW(nullptr), nullptr);
  SetWindowLongPtr(m_host_hwnd, GWLP_USERDATA, (LONG_PTR)this);

  CreateGameWindow(m_host_hwnd);
  CreateControlWindow(m_host_hwnd);

  ShowWindow(m_host_hwnd, SW_SHOW);
  UpdateWindow(m_host_hwnd);
}

void Application::CreateControlWindow(HWND parent_hwnd) {
  DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
  RECT game_rect;
  ::GetClientRect(m_game_hwnd, &game_rect);
  LONG pos_x = game_rect.right + 10;
  LONG pos_y = game_rect.top + 40;
  CreateWindowW(L"Button", L"发送消息到小程序", style,
    pos_x, pos_y, 140, 40, parent_hwnd, (HMENU)SEND_MSG_BTN_ID, nullptr, nullptr);

  pos_y += 50;
  CreateWindowW(L"Static", L"待发送到小程序的消息", style,
    pos_x, pos_y, 160, 20, parent_hwnd, nullptr, nullptr, nullptr);

  pos_y += 20;
  m_send_edit_hwnd = CreateWindowW(L"Edit", L"消息内容123", style | WS_VSCROLL | WS_BORDER | ES_MULTILINE,
    pos_x, pos_y, 280, 200, parent_hwnd, nullptr, nullptr, nullptr);

  pos_y += 220;
  CreateWindowW(L"Static", L"接收到小程序的消息", style,
    pos_x, pos_y, 160, 20, parent_hwnd, nullptr, nullptr, nullptr);

  pos_y += 20;
  m_recv_edit_hwnd = CreateWindowW(L"Edit", L"", style | WS_VSCROLL | WS_BORDER | ES_MULTILINE | ES_READONLY,
    pos_x, pos_y, 280, 200, parent_hwnd, nullptr, nullptr, nullptr);
}

void Application::GetWindowPixels() {
  if (!IsWindow(m_game_hwnd)) {
    m_logger->info("Invalid window handle");
    return;
  }

  RECT rect;
  GetClientRect(m_game_hwnd, &rect);
  int width = rect.right - rect.left;
  int height = rect.bottom - rect.top;

  HDC hWindowDC = GetDC(m_game_hwnd);
  HDC hMemoryDC = CreateCompatibleDC(hWindowDC);
  HBITMAP hBitmap = CreateCompatibleBitmap(hWindowDC, width, height);
  HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hMemoryDC, hBitmap));

  BitBlt(hMemoryDC, 0, 0, width, height, hWindowDC, 0, 0, SRCCOPY);

  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height;  // top-down bitmap
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  size_t imageSize = width * height * 4;

  static std::string glb_tmpBuf;
  if (glb_tmpBuf.size() != width * height * 4) {
    glb_tmpBuf.resize(width * height * 4, 0);
  }
  GetDIBits(hMemoryDC, hBitmap, 0, height, &glb_tmpBuf[0], &bmi, DIB_RGB_COLORS);
  {
    std::lock_guard<std::mutex> l(m_videoBufMutex);
    if (m_videoBuf.size() != glb_tmpBuf.size()) {
      m_videoBuf.resize(glb_tmpBuf.size(), 0);
    }

    libyuv::ARGBToABGR((uint8_t*)glb_tmpBuf.data(), width * 4, (uint8_t*)m_videoBuf.data(), width * 4, width, height);
    // 将 alpha 通道设置为不透明
    for (size_t i = 0; i < width * height; i++) {
        size_t offset = i * 4;
        m_videoBuf[offset + 3] = 255;
    }
    DumpImage(width, height, m_videoBuf.data(), width * 4);
  }

  SelectObject(hMemoryDC, hOldBitmap);
  DeleteObject(hBitmap);
  DeleteDC(hMemoryDC);
  ReleaseDC(m_game_hwnd, hWindowDC);
}

void Application::DumpImage(int32_t width, int32_t height, const char* buf, int32_t bufSize) 
{
  fs::path currentPath = fs::current_path();
  fs::path flagFile = currentPath.append("dump_image");
  if (!fs::exists(flagFile))
    return;

  static int64_t glb_image_frame = 0;
  std::string imageName = R"(dump_image_)";
  imageName = imageName.append(std::to_string(glb_image_frame));
  imageName = imageName.append(R"(.png)");
  fs::path targetPath = currentPath.append(imageName);
  if (glb_image_frame++ % 100 == 0) 
  {
    stbi_write_png(targetPath.u8string().c_str(), width, height, 4, buf, width * 4);
  }
}

void Application::SendEditCustomMsg() {
  wchar_t edit_wstr[1024]{0};
  ::GetWindowText(m_send_edit_hwnd, edit_wstr, _countof(edit_wstr));
  auto edit_utf8_str = rtc::ToUtf8(edit_wstr);
  SendBizMsg("SendMessageToApplet", BuildSendMessageToApplet(edit_utf8_str));
}

void Application::UpdateRecvEditMsg(const std::string& msg) {
  auto msg_str = new std::string(msg);
  ::PostMessage(m_host_hwnd, WM_RECV_UPDATE_MSG, 0, (LPARAM)msg_str);
}

void Application::UpdateRecvEdit(const std::string& msg) {
  std::wstring msg_wstr = rtc::ToUtf16(msg.c_str());
  ::SetWindowText(m_recv_edit_hwnd, msg_wstr.c_str());
}
