# 项目(DanMuClock)演示C# Unity游戏接入CPPSDK，与主播端、小程序前端通信及投射图层到主播端流程

## 重要文件说明
* MediaPiplelineControllerSDK.cs：C#包装CPP SDK，包括一些初始化或通信细节处理，开发者可不做调整修改直接引入到unity工程中使用即可
* Clock.cs：示例用例类，其引入MediaPiplelineControllerSDK.cs文件，并解释了初始化、更新游戏画面、通信调用或回调注册、协议包装、回包解析等，具体可以参照该源码实现
