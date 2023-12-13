using AOT;
using LitJson;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using UnityEngine;

// MediaPipelineControllerSDK 的 Unity 封装层
// 该SDK的作用:
// 1. 用于与主播端、前端小程序间的通信、接口调用
// 2. 用于投屏到主播端可自定义或多个图层投屏
// 注：
//   若业务侧不处理多游戏图层或只投屏当前游戏画面本身，则可忽略调用UpdateAnchorLayer及cpp_pipeline_add_*等系列接口，主播端会自动采集游戏画面)
//   业务侧可根据自身需要使用这两种能力，也可只用其中一种，比如：仅与前端小程序或主播端间的通信或查询接口
//   推送游戏画面到主播端时，推荐优先采用纹理方式，此外若需要播放游戏音频，可以直接本地播放即可，主播端会采集
// 外部游戏Game、小程序一体化交互接口文档：https://github.com/weigod/game_sdk_demo/blob/master/game_interface.md
// 小程序接口SDK文档：https://dev.huya.com/docs/miniapp/dev/sdk/
// C++或C#包装接口接入文档：https://github.com/weigod/game_sdk_demo

namespace Huya.USDK.MediaController
{
    // sdk 生成的自定义数据事件
    public struct CustomDataEvent
    {
        public ulong uid;
        public string streamUrl;
        public string streamUUID;
        public string streamUUIDS;
        public string customData; // json 字符串(重要)
    }

    // 原始视频数据帧(旧)
    public struct RawVideoFrame
    {
        public int frameID;
        public int width;
        public int height;
        public int lineSize;
        public int pixelFormat;
        public IntPtr pixelData;
        public int fps;
    }

    // 音频数据帧信息(旧)
    public struct PCMInfo
    {
        public int sampleRate;
        public int channelLayout;
        public int sampleCount;
        public int audioFormat;
    }

    // 视频帧
    public class VideoFrame
    {
        public string streamUUID;       // 流地址名称
        public IntPtr nativeTex;        // 游戏画面原生纹理，优先使用。无效时才使用 data
        public int width;               // 游戏画面宽度
        public int height;              // 游戏画面高度
        public TextureFormat format;    // 支持的有效值: TextureFormat.RGBA32, TextureFormat.BGRA32
        public byte[] data;             // 游戏画面数据
        public long pts;                // pts
        public byte[] customData;       // 自定义数据
        public int customDataSize;      // 自定义数据内容大小, 可使用 customData.Length 获取大小
    }

    // 音频帧
    public class AudioFrame
    {
        public int sampleRate;          // 采样率
        public int sampleCount;         // 采样点数
        public byte[] data;             // 音频数据
        public int dataSize;            // 音频数据内容大小, 可使用 data.Length 获取大小
        public long pts;                // pts
        public byte[] customData;       // 自定义数据
    }

    // 日志级别
    public enum LogLevel
    {
        Debug,
        Info,
        Warning,
        Error
    }

    // 输出流地址名称列表(主要用在输出多个游戏画面时用到,一个画面一条流，一条流分别含音频流和视频流)
    [Serializable]
    public class OutputStreams
    {
        public List<String> cpp_stream_uuids;
    }

    /// 封装SDK包装类实现
    public class MediaPiplelineControllerSDK
    {
        /// 自定义流数据回调委托。会返回某些配置，如主播端分辨率等，业务侧可根据需要获取。
        /// <param name="customDataEvent">包含自定义数据</param>
        public delegate void OnStreamCustomData(CustomDataEvent customDataEvent);

        /// 日志委托，若业务侧有需要，可初始化该对象以写业务日志
        /// <param name="message"></param>
        /// <param name="level"></param>
        public delegate void OnStreamLog(string message, LogLevel level);

        /// 小程序Invoke 与游戏间的消息通道委托
        /// <param name="apiName"></param>
        /// <param name="rsp"></param>
        /// <param name="reqId"></param>
        /// <param name="err"></param>
        public delegate void OnAppletInvokeRspMsg(string apiName, string rsp, string reqId, string err);

        /// 小程序Invoke Listen 与游戏间的消息通道委托
        /// <param name="apiName"></param>
        /// <param name="message"></param>
        /// <param name="cbId"></param>
        public delegate void OnAppletInvokeListenRspMsg(string apiName, string message, string cbId);

        /// 小程序与游戏间的通用消息通道委托
        /// <param name="message"></param>
        public delegate void OnAppletMsg(string message);

        /// 主播端与游戏间的通用消息通道委托
        /// <param name="eventName"></param>
        /// <param name="messageJsonData"></param>
        public delegate void OnAnchorMsg(string eventName, JsonData messageJsonData);

        /// 自定义流数据回调委托。会返回某些配置，如主播端分辨率等，业务侧可根据需要获取。
        /// <param name="userData"></param>
        /// <param name="pipelineId"></param>
        /// <param name="dataEvent"></param>
        private delegate void CustomDataCallback(IntPtr userData, string pipelineId, CustomDataEvent dataEvent);
        private CustomDataCallback customDataCallback;

        /// 通道数据消息回调委托
        /// <param name="userData"></param>
        /// <param name="texturePtr"></param>
        /// <param name="videoData"></param>
        private delegate void CppCallChannelMsgCallBack(IntPtr userData, string msg, uint len);
        private CppCallChannelMsgCallBack cppCallChannelMsgCallBack;

        // 以下为用到的主要CPP SDK导出接口
        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_context_init3(int thriftPort, string jobId, string logDir, int thriftListenPort, bool enableDumpCapture);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern void cpp_context_uninit();

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_set_custom_data_cb(IntPtr cb, IntPtr userData);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_pipeline_dump_sharedtexture(IntPtr cb, IntPtr userData, long sharedHandle, IntPtr context, IntPtr contextTexture);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_pipeline_add_raw(string streamUUID, RawVideoFrame frame, IntPtr captureStartTickMs, ref long pts);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_pipeline_add_texture(string streamUUID, IntPtr context, IntPtr texturePtr, int textureType, ref long pts, ref long fps);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_pipeline_add_texture_jce(string streamUUID, IntPtr context, IntPtr texturePtr, int textureType, ref long pts, ref long fps, IntPtr customData, uint customDataSize);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_pipeline_add_pcm(string streamUUID, PCMInfo pcm, IntPtr strData, uint strDataSize, ref long pts);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_pipeline_add_pcm_jce(string streamUUID, PCMInfo pcm, IntPtr strData, uint strDataSize, ref long pts, IntPtr customData, uint customDataSize);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_call_channel_msg(IntPtr sendBuf, uint sendBufLen);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_set_channel_msg_cb(IntPtr cb, IntPtr userData);

        // 外部回调函数成员
        public OnStreamCustomData onStreamCustomData;
        public OnStreamLog onStreamLog;
        public OnAppletInvokeRspMsg onAppletInvokeRspMsg;
        public OnAppletInvokeListenRspMsg onAppletInvokeListenRspMsg;
        public OnAppletMsg onAppletMsg;
        public OnAnchorMsg onAnchorMsg;

        // 内部成员
        private int mOutVideoFrameID = 0;
        private int mOutVideoFPS = 0;
        private GCHandle mHandleForThis;
        private List<string> mOutputStreams;
        private Dictionary<string, string> mMapFromInputStreamToOutputStream;
        private Queue<Action> mCustomQueue;
        private Queue<Action> mChannelMsgQueue;

        /// 初始化SDK
        public int Init()
        {
            // 获取日志路径，初始化sdk需要，业务也可以使用该路径用来生成业务日志到该路径下便于统一处理
            var logDirEnv = GetLogDirPath();

            var port = Environment.GetEnvironmentVariable("CPP_PORT");
            if (port != null)
            {
                var jobId = Environment.GetEnvironmentVariable("CPP_JOB_ID");
                var listenPort = Environment.GetEnvironmentVariable("CPP_LISTEN_PORT"); // 用于IDE调试

                // 获取推流地址
                var streamUUIDs = Environment.GetEnvironmentVariable("CPP_STREAM_UUIDS");
                if (streamUUIDs == null)
                {
                    onStreamLog?.Invoke("there is no env variable named CPP_STREAM_UUIDS", LogLevel.Error);
                    return -1;
                }

                var bytes = Convert.FromBase64String(streamUUIDs);
                var jsonString = Encoding.UTF8.GetString(bytes);
                var streams = JsonUtility.FromJson<OutputStreams>(jsonString);

                int thriftListenPort = 0;
                if (!string.IsNullOrEmpty(listenPort))
                {
                    thriftListenPort = int.Parse(listenPort);
                }
                else
                {
                    onStreamLog?.Invoke($"MediaPipelineControllerSDK thriftListenPort is null ", LogLevel.Warning);
                }
                return InitImpl(Int32.Parse(port), thriftListenPort, jobId, logDirEnv, streams.cpp_stream_uuids);
            }
            return InitImpl(0, 0, "", logDirEnv, new List<string>());
        }

        /// 初始化实现
        /// <param name="thriftPort">大于 0 的任意值。本地测试时不会用到，正式环境下从环境变量获取。</param>
        /// <param name="thriftListenPort">大于 0 的任意值。本地测试时不会用到，正式环境下从环境变量获取。</param>
        /// <param name="jobId">任意非空字符串。本地测试时不会用到，正式环境下从环境变量获取。</param>
        /// <param name="logDir">日志路径。null 时在 dll 同级目录存放日志。</param>
        /// <param name="oStreams">输出流名称</param>
        private int InitImpl(int thriftPort, int thriftListenPort, string jobId, string logDir, List<string> oStreams)
        {
            onStreamLog?.Invoke($"MediaPipelineControllerSDK Init, thriftPort={thriftPort},thriftListenPort={thriftListenPort}, jobId={jobId}, logDir={logDir}", LogLevel.Info);
            mCustomQueue = new Queue<Action>();
            mChannelMsgQueue = new Queue<Action>();
            mOutputStreams = oStreams;
            mMapFromInputStreamToOutputStream = new Dictionary<string, string>();
            mHandleForThis = GCHandle.Alloc(this);
            mOutVideoFrameID = 0;
            int ret = 0;
            do
            {
                ret = cpp_context_init3(thriftPort, jobId, logDir, thriftListenPort, false);
                if (ret != 0)
                {
                    onStreamLog?.Invoke($"cpp_context_init3 error, {ret}", LogLevel.Error);
                    break;
                }

                customDataCallback = new CustomDataCallback(OnCustomDataImpl);
                ret = cpp_set_custom_data_cb(Marshal.GetFunctionPointerForDelegate(customDataCallback), GCHandle.ToIntPtr(mHandleForThis));
                if (ret != 0)
                {
                    onStreamLog?.Invoke($"cpp_set_custom_data_cb error, {ret}", LogLevel.Error);
                    break;
                }

                cppCallChannelMsgCallBack = new CppCallChannelMsgCallBack(OnCppCallChannelMsgCallBack);
                ret = cpp_set_channel_msg_cb(Marshal.GetFunctionPointerForDelegate(cppCallChannelMsgCallBack), GCHandle.ToIntPtr(mHandleForThis));
                if (ret != 0)
                {
                    onStreamLog?.Invoke($"cpp_set_channel_msg_cb error, {ret}", LogLevel.Error);
                    break;
                }

            } while (false);
            onStreamLog?.Invoke($"MediaPipelineControllerSDK Init finished, {ret}", LogLevel.Info);
            return ret;
        }

        /// 反初始化SDK
        public void Uninit()
        {
            cpp_context_uninit();
            mHandleForThis.Free();
        }

        // 更新通道消息，业务侧需要在Update中调用或某个线程中定时调用
        public void Update()
        {
            lock (mCustomQueue)
            {
                while (mCustomQueue.Count > 0)
                {
                    mCustomQueue.Dequeue().Invoke();
                }
            }

            lock (mChannelMsgQueue)
            {
                while (mChannelMsgQueue.Count > 0)
                {
                    mChannelMsgQueue.Dequeue().Invoke();
                }
            }
        }

        /// <summary>
        /// 返回配置的输出流地址列表。
        /// Init 之后调用。
        /// </summary>
        public List<string> OutputStreams
        {
            get => mOutputStreams;
            set => mOutputStreams = value;
        }

        public void AddOutputStream(string streams)
        {
            if (mOutputStreams != null)
            {
                mOutputStreams.Add(streams);
            }
        }

        public void RemoveOutputStream(string streams)
        {
            if (mOutputStreams != null)
            {
                if (mOutputStreams.Count > 0)
                {
                    mOutputStreams.Remove(streams);
                }
            }
        }

        /// 指定推流 FPS
        /// <param name="fps"></param>
        public void SetOutFPS(int fps)
        {
            mOutVideoFPS = fps;
        }

        /// 推视频流
        /// 数据内存由 sdk 负责释放。
        /// <param name="srcStreamUUID">目的流地址，即要发送的 frame 是从哪条拉流处理而来的</param>
        /// <param name="frame"></param>
        public int SendVideo(string srcStreamUUID, VideoFrame frame)
        {
            var outputStream = GetOutputStreamByInputStream(srcStreamUUID);
            return SendVideoTo(outputStream, frame);
        }

        /// 推视频流，视频流为画面数据。仅有一条输出流时使用。
        /// <param name="frame"></param>
        public int SendOneVideo(VideoFrame frame)
        {
            if (mOutputStreams.Count == 0)
            {
                onStreamLog?.Invoke("There is no output stream, please check your configuration.", LogLevel.Error);
                return -1;
            }
            if (mOutputStreams.Count > 1)
            {
                onStreamLog?.Invoke($"There are more than one output streams in configuration but you don't specify one, use the first output stream.", LogLevel.Warning);
            }
            return SendVideoTo(mOutputStreams[0], frame);
        }

        /// 将视频流推到指定流地址。
        /// <param name="dstStreamUUID">目的流地址。必须存在于 OutputStreams 列表中。</param>
        /// <param name="frame"></param>
        public int SendVideoTo(string dstStreamUUID, VideoFrame frame)
        {
            RawVideoFrame rawFrame = new RawVideoFrame();
            rawFrame.frameID = ++mOutVideoFrameID;
            rawFrame.width = frame.width;
            rawFrame.height = frame.height;
            rawFrame.lineSize = frame.data.Length / frame.height;
            rawFrame.pixelFormat = frame.format == TextureFormat.RGBA32 ? 3 : 1;
            GCHandle dataHandle = GCHandle.Alloc(frame.data, GCHandleType.Pinned);
            rawFrame.pixelData = dataHandle.AddrOfPinnedObject();
            rawFrame.fps = mOutVideoFPS;
            int ret = cpp_pipeline_add_raw(dstStreamUUID, rawFrame, IntPtr.Zero, ref frame.pts);
            dataHandle.Free();
            if (ret != 0)
            {
                onStreamLog?.Invoke($"cpp_pipeline_add_raw error, {ret}", LogLevel.Error);
            }
            return ret;
        }

        /// 推视频流，视频流为纹理数据，推出一个指定纹理所指代的视频帧。
        /// 只支持 DX11 纹理
        /// 必须在 UI 线程调用，保证 DX 上下文安全
        /// <param name="srcStreamUUID">源流名称，即要发送的纹理是从哪条拉流处理而来的</param>
        /// <param name="nativeTexturePtr">指代 ID3D11Resource *。可通过 Texture.GetNativeTexturePtr 获取。</param>
        /// <param name="pts"></param>
        /// <param name="customData"></param>
        public int SendVideo(string srcStreamUUID, IntPtr nativeTexturePtr, long pts, byte[] customData = null)
        {
            long fps = mOutVideoFPS;
            var outputStream = GetOutputStreamByInputStream(srcStreamUUID);
            if (customData != null)
            {
                GCHandle dataPined = GCHandle.Alloc(customData, GCHandleType.Pinned);
                IntPtr dataPtr = dataPined.AddrOfPinnedObject();
                int ret = cpp_pipeline_add_texture_jce(outputStream, IntPtr.Zero, nativeTexturePtr, 0, ref pts, ref fps, dataPtr, (uint)customData.Length);
                dataPined.Free();
                return ret;
            }
            return cpp_pipeline_add_texture(outputStream, IntPtr.Zero, nativeTexturePtr, 0, ref pts, ref fps);
        }

        /// 推视频流，视频流为纹理数据。仅有一条输出流时使用。
        /// <param name="nativeTexturePtr"></param>
        /// <param name="pts"></param>
        public int SendOneVideo(IntPtr nativeTexturePtr, long pts)
        {
            if (mOutputStreams.Count == 0)
            {
                onStreamLog?.Invoke("There is no output stream, please check your configuration.", LogLevel.Error);
                return -1;
            }
            if (mOutputStreams.Count > 1)
            {
                onStreamLog?.Invoke($"There are more than one output streams in configuration but you don't specify one, use the first output stream.", LogLevel.Warning);
            }
            return SendVideoTo(mOutputStreams[0], nativeTexturePtr, pts);
        }

        /// 将视频流推到指定流地址。
        /// <param name="dstStreamUUID">目的流地址。必须存在于 OutputStreams 列表中。</param>
        /// <param name="nativeTexturePtr"></param>
        /// <param name="pts"></param>
        /// <param name="customData"></param>
        public int SendVideoTo(string dstStreamUUID, IntPtr nativeTexturePtr, long pts, byte[] customData = null)
        {
            long fps = mOutVideoFPS;
            if (customData != null)
            {
                GCHandle dataPined = GCHandle.Alloc(customData, GCHandleType.Pinned);
                IntPtr dataPtr = dataPined.AddrOfPinnedObject();
                int ret = cpp_pipeline_add_texture_jce(dstStreamUUID, IntPtr.Zero, nativeTexturePtr, 0, ref pts, ref fps, dataPtr, (uint)customData.Length);
                dataPined.Free();
                return ret;
            }
            return cpp_pipeline_add_texture(dstStreamUUID, IntPtr.Zero, nativeTexturePtr, 0, ref pts, ref fps);
        }

        /// 推音频流
        /// 数据内存由 sdk 负责释放。
        /// <param name="srcStreamUUID">目的流地址，即要发送的 frame 是从哪条拉流处理而来的</param>
        /// <param name="frame"></param>
        public int SendAudio(string srcStreamUUID, AudioFrame frame)
        {
            var outputStream = GetOutputStreamByInputStream(srcStreamUUID);
            return SendAudioTo(outputStream, frame);
        }

        /// 推音频流。仅有一条输出流时使用。
        /// <param name="frame"></param>
        public int SendAudio(AudioFrame frame)
        {
            if (mOutputStreams.Count == 0)
            {
                onStreamLog?.Invoke("There is no output stream, please check your configuration.", LogLevel.Error);
                return -1;
            }
            if (mOutputStreams.Count > 1)
            {
                onStreamLog?.Invoke($"There are more than one output streams in configuration but you don't specify one, use the first output stream.", LogLevel.Warning);
            }
            return SendAudioTo(mOutputStreams[0], frame);
        }

        /// 将音频流推到指定流地址。
        /// <param name="dstStreamUUID">目的流地址。必须存在于 OutputStreams 列表中。</param>
        /// <param name="frame"></param>
        /// <param name="customData"></param>
        public int SendAudioTo(string dstStreamUUID, AudioFrame frame, byte[] customData = null)
        {
            PCMInfo pcm = new PCMInfo();
            pcm.sampleRate = frame.sampleRate;
            pcm.sampleCount = frame.sampleCount;
            pcm.channelLayout = 2;
            pcm.audioFormat = 0;
            GCHandle dataHandle = GCHandle.Alloc(frame.data, GCHandleType.Pinned);
            int ret;
            if (customData != null)
            {
                GCHandle customHandle = GCHandle.Alloc(customData, GCHandleType.Pinned);
                ret = cpp_pipeline_add_pcm_jce(dstStreamUUID, pcm, dataHandle.AddrOfPinnedObject(), (uint)frame.dataSize, ref frame.pts, customHandle.AddrOfPinnedObject(), (uint)customData.Length);
                customHandle.Free();
            }
            else
            {
                ret = cpp_pipeline_add_pcm(dstStreamUUID, pcm, dataHandle.AddrOfPinnedObject(), (uint)frame.dataSize, ref frame.pts);
            }
            dataHandle.Free();
            if (ret != 0)
            {
                onStreamLog?.Invoke($"cpp_pipeline_add_pcm error, {ret}", LogLevel.Error);
            }
            return ret;
        }

        public static string GetLogDirPath()
        {
            var logDirPath = Environment.GetEnvironmentVariable("LOCAL_LOG_DIR");
            if (logDirPath == null)
            {
                logDirPath = System.Environment.CurrentDirectory;
            }

            logDirPath = Encoding.UTF8.GetString(Encoding.Default.GetBytes(logDirPath));
            return logDirPath;
        }

        private string GetOutputStreamByInputStream(string inputStream)
        {
            if (!mMapFromInputStreamToOutputStream.ContainsKey(inputStream))
            {
                mMapFromInputStreamToOutputStream[inputStream] = mOutputStreams[0];
                mOutputStreams.RemoveAt(0);
            }
            return mMapFromInputStreamToOutputStream[inputStream];
        }

        // IL2CPP 不支持类成员方法委托的 marshal
        [MonoPInvokeCallback(typeof(CustomDataCallback))]
        private static void OnCustomDataImpl(IntPtr userData, string pipelineId, CustomDataEvent customDataEvent)
        {
            var h = GCHandle.FromIntPtr(userData);
            var sdk = (h.Target as MediaPiplelineControllerSDK);
            sdk.OnCustomData(customDataEvent);
        }

        private void OnCustomData(CustomDataEvent customDataEvent)
        {
            onStreamLog?.Invoke($"MediaPipelineControllerSDK CallBack OnCustomData {customDataEvent.customData} LogTime = {GetLogTime()} Thread =  {Thread.CurrentThread.ManagedThreadId.ToString()}", LogLevel.Info);
            if (onStreamCustomData != null)
            {
                lock (mCustomQueue)
                {
                    mCustomQueue.Enqueue(() =>
                    {
                        onStreamCustomData(customDataEvent);
                    });
                }
            }
        }

        private static string GetLogTime()
        {
            string str = DateTime.Now.ToString("HH:mm:ss.fff") + "";
            return str;
        }

        [MonoPInvokeCallback(typeof(CppCallChannelMsgCallBack))]
        private static void OnCppCallChannelMsgCallBack(IntPtr userData, string msg, uint len)
        {
            var h = GCHandle.FromIntPtr(userData);
            var sdk = (h.Target as MediaPiplelineControllerSDK);
            sdk.OnRecvChannelMsg(msg);
        }

        // 通信接口回调(接收主播端或前端小程序发来的请求)
        private void OnRecvChannelMsg(string msg)
        {
            lock (mChannelMsgQueue)
            {
                mChannelMsgQueue.Enqueue(() =>
                {
                    DealStreamChannelMsgData(msg);
                });
            }
        }

        // 通信基础接口(发送请求至主播端或前端小程序，过于底层，一般不直接调用)
        public void SendChannelMsg(string jsonStr)
        {
            if (jsonStr == null)
                return;

            byte[] sendData = System.Text.Encoding.UTF8.GetBytes(jsonStr);
            GCHandle dataPined = GCHandle.Alloc(sendData, GCHandleType.Pinned);
            IntPtr dataPtr = dataPined.AddrOfPinnedObject();
            int ret = cpp_call_channel_msg(dataPtr, (uint)sendData.Length);
            dataPined.Free();
            if (ret != 0)
            {
                onStreamLog?.Invoke($"cpp_call_channel_msg error {ret}", LogLevel.Error);
                return;
            }
        }

        // 发送请求至主播端或前端小程序通用接口消息
        public void SendGeneralMsg(string eventName, string reqId, JsonData messageJsonData)
        {
            JsonData jsonChannelMsgData = new JsonData();
            jsonChannelMsgData["eventName"] = eventName;
            jsonChannelMsgData["reqId"] = reqId;
            jsonChannelMsgData["message"] = messageJsonData;
            var jsonChannelMsgStr = jsonChannelMsgData.ToJson();
            SendChannelMsg(jsonChannelMsgStr);
        }

        // 定制：发送前端小程序Invoke请求消息(即小程序JS的请求或操作相关接口)
        public void SendAppletInvokeReqMsg(string appletApiName, List<string> appletApiParamArray, string reqId)
        {
            JsonData messageJsonData = new JsonData();
            JsonData msgJsonData = new JsonData();
            msgJsonData["type"] = "CALL_JS_SDK_REQ";

            var paramsJsonData = new JsonData();
            paramsJsonData.SetJsonType(JsonType.Array);
            foreach (var appletApiParam in appletApiParamArray)
            {
                paramsJsonData.Add(appletApiParam);
            }

            JsonData payloadJsonData = new JsonData();
            payloadJsonData["reqId"] = reqId;
            payloadJsonData["apiName"] = appletApiName;
            payloadJsonData["params"] = paramsJsonData;
            msgJsonData["payload"] = payloadJsonData;

            messageJsonData["msg"] = msgJsonData.ToJson();
            SendGeneralMsg("SendMessageToApplet", reqId, messageJsonData);
        }

        // 定制：发送小程序调用监听消息接口(即小程序JS的监听相关接口)
        public void SendAppletInvokeListenMsg(
            string appletApiName, List<string> appletApiParamArray,
            string reqId, string listenOnOff, string eventType)
        {
            JsonData messageJsonData = new JsonData();
            JsonData msgJsonData = new JsonData();
            msgJsonData["type"] = "CALL_JS_SDK_REQ";

            var paramsJsonData = new JsonData();
            paramsJsonData.SetJsonType(JsonType.Array);
            foreach (var param in appletApiParamArray)
            {
                paramsJsonData.Add(param);
            }

            JsonData payloadJsonData = new JsonData();
            payloadJsonData["reqId"] = reqId;
            payloadJsonData["apiName"] = appletApiName;
            payloadJsonData["params"] = paramsJsonData;
            payloadJsonData["callType"] = listenOnOff;
            payloadJsonData["eventType"] = eventType;
            msgJsonData["payload"] = payloadJsonData;

            messageJsonData["msg"] = msgJsonData.ToJson();
            SendGeneralMsg("SendMessageToApplet", reqId, messageJsonData);
        }

        private void DealStreamChannelMsgData(string message)
        {
            if (message == null)
            {
                return;
            }

            JsonData jsonData = JsonMapper.ToObject(message);
            if (!jsonData.ContainsKey("message") ||
               !jsonData.ContainsKey("eventName") ||
               !jsonData.ContainsKey("reqId") ||
               !jsonData.ContainsKey("res"))
            {
                return;
            }

            var reqId = (string)jsonData["reqId"];
            var res = (int)jsonData["res"];

            // 针对不同eventName，解析不同的message内容，由文档说明对应解析即可
            JsonData messageJsonData = jsonData["message"];
            var eventName = (string)jsonData["eventName"];
            if (eventName == "SendMessageToApplet_Callback") // 一般可忽略
            {
                //if (messageJsonData.ContainsKey("msg"))
                //{
                //    var msg = (string)messageJsonData["msg"];
                //    Debug.Log("get applet channel msg, eventName: " + eventName + ", msg: " + msg);
                //}
            }
            else if (eventName == "OnAppletMessage") // 前端小程序发过来消息
            {
                if (messageJsonData.ContainsKey("msg"))
                {
                    var msg = (string)messageJsonData["msg"];
                    // 需区分小程序invoke调用通告或常规通告消息
                    JsonData msgJsonData = JsonMapper.ToObject(msg);
                    if (msgJsonData.IsObject)
                    {
                        ParseAppletInvokeMessage(msgJsonData);
                    }
                    else
                    {
                        ParseAppletMessage(msg);
                    }
                }
            }
            else
            {
                if (onAnchorMsg != null)
                {
                    onAnchorMsg(eventName, messageJsonData);
                }
            }
        }

        private void ParseAppletInvokeMessage(JsonData msgJsonData)
        {
            // 小程序invoke调用通告
            if (msgJsonData.ContainsKey("name") &&
                "GameMsg" == (string)msgJsonData["name"] &&
                msgJsonData.ContainsKey("message"))
            {
                JsonData internalMessageJsonData = msgJsonData["message"];
                if (internalMessageJsonData.ContainsKey("type") &&
                    internalMessageJsonData.ContainsKey("payload"))
                {
                    var type = (string)internalMessageJsonData["type"];
                    if ("CALL_JS_SDK_RSP" == type) // 小程序invoke调用应答通告
                    {
                        var playloadJsonData = internalMessageJsonData["payload"];
                        if (playloadJsonData.ContainsKey("apiName") &&
                            playloadJsonData.ContainsKey("reqId"))
                        {
                            var apiName = (string)playloadJsonData["apiName"];
                            var internalReqId = (string)playloadJsonData["reqId"];
                            var rsp = "";
                            var err = "";
                            if (playloadJsonData.ContainsKey("rsp"))
                            {
                                rsp = (string)playloadJsonData["rsp"];
                            }

                            if (playloadJsonData.ContainsKey("err"))
                            {
                                var errJsonData = playloadJsonData["err"];
                                err = errJsonData.ToJson();
                                if (err == "")
                                {
                                    err = "occur unexpected error.";
                                }
                            }

                            if (onAppletInvokeRspMsg != null)
                            {
                                onAppletInvokeRspMsg(apiName, rsp, internalReqId, err);
                            }
                        }
                    }
                    else if ("CALL_JS_SDK_EVENT" == type) // 小程序invoke监听通告
                    {
                        var playloadJsonData = internalMessageJsonData["payload"];
                        if (playloadJsonData.ContainsKey("apiName") &&
                            playloadJsonData.ContainsKey("message"))
                        {
                            var apiName = (string)playloadJsonData["apiName"];
                            var messageStr = (string)playloadJsonData["message"];
                            var cbId = "";
                            if (playloadJsonData.ContainsKey("cbId"))
                            {
                                cbId = (string)playloadJsonData["cbId"];
                            }
                            
                            if (onAppletInvokeListenRspMsg != null)
                            {
                                onAppletInvokeListenRspMsg(apiName, messageStr, cbId);
                            }
                        }
                    }
                }
            }
        }

        private void ParseAppletMessage(string message)
        {
            if (onAppletMsg != null)
            {
                onAppletMsg(message);
            }
        }
    }
}