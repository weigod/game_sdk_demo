/*!
 * \file MediaDefs.h
 * \date 2020/11/14 10:05
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
#ifndef MEDIADEFS_H_
#define MEDIADEFS_H_

#include <stdint.h>
#include <string>

enum MediaMode
{
  kMediaMode_Both = 0,
  kMediaMode_AudioOnly,
  kMediaMode_VideoOnly,
  kMediaMode_Count,
  kMediaMode_Max = 255
};

enum HuyaClientType
{
  kHuyaClientType_PC = 0,
  kHuyaClientType_CloudGamePush = 100,
  kHuyaClientType_CloudGamePull = 101,
  kHuyaClientType_VirtualActor = 105,
};
////////////////////////////////////////////////////////////////////////
// StartArgs
// windows HUYA 推拉流需要填一个client type的启动参数
/*
云游戏，小游戏
推流用 YCMEDIA_C_ENUM_ClientType_PC_YUNYOUXI_PUSH
拉流用 YCMEDIA_C_ENUM_ClientType_PC_YUNYOUXI_PULL
用来加工推拉流场景，都填
YCMEDIA_C_ENUM_ClientType_PC_YY

目前的区别是@2020年11月9日 09点54分：
前2个会在服务器上使用独立的vp进程，暂无其它差别.
*/
struct HuyaStartArgs
{
  uint32_t clientType = 0;
  uint32_t mode = 0;
};

struct HuyaLiveArgs
{
  std::string liveId;
  int64_t UStmpc = 0;
  int32_t mode = 0; //1=low latency
  std::string password;
  std::string cndStreamName;
};

#endif  // MEDIADEFS_H_
