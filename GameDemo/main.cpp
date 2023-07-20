#include "Application.h"
#ifdef WIN32
#include <windows.h>
#endif

#if defined(WIN32) && defined(NDEBUG)
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif
int main(int argc, char** argv)
{
  //进程工作目录指向插件所在位置
  fs::path exePath = argv[0];
  std::error_code ec;
  fs::current_path(exePath.parent_path().string(), ec);

  Application app;
  if (app.Init(argc, argv))
    return 0;

  int ret = app.Run();
  app.UnInit();
  return ret;
}