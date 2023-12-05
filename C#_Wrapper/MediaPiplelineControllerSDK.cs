using AOT;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using UnityEngine;

namespace Huya.USDK.MediaController
{
    public class VideoFrame
    {
        public string streamUUID;
        public IntPtr nativeTex; // 原生纹理，优先使用。无效时才使用 data
        public int width;
        public int height;
        public TextureFormat format; // valid values: TextureFormat.RGBA32, TextureFormat.BGRA32
        public byte[] data;
        public long pts;
        public byte[] customData;
        public int customDataSize; // use this instead of customData.Length to get the length of customData
    }

    public class AudioFrame
    {
        public int sampleRate;
        public int sampleCount;
        public byte[] data;
        public int dataSize;
        public long pts;
        public byte[] customData;
    }

    public enum LogLevel
    {
        Debug,
        Info,
        Warning,
        Error
    }

    [Serializable]
    public class OutputStreams
    {
        public List<String> cpp_stream_uuids;
    }

    class VideoCache
    {
        public VideoFrame frame = new VideoFrame();
        public long sharedHandle; // 共享纹理。优先使用共享纹理，为 0 时才使用 frame
        public byte[] pixelDataBuffer;
        public byte[] customDataBuffer = new byte[1024];
        public bool used = false;
    }

    /// <summary>
    /// MediaPipelineControllerSDK 的 Unity 封装层。
    /// 
    /// 本地调试时，拉流、推流的各种配置存放在 ControllerSDKDebug.conf 文件。而正式发布时，是没有这个配置文件的。
    /// 
    /// 基于业务进行了深度优化：
    /// 1. Video、Audio、Custom 三种数据分别存储，避免使用同一把锁引起的不必要竞争；
    /// 2. Video 数据不做队列，仅保存所在流的最后一个视频帧，避免了频繁分配内存。
    /// </summary>
    public class MediaPiplelineControllerSDK
    {
        /// <summary>
        /// 自定义数据回调。会返回初始化信令库所用的配置。如果不需要可以不设置。
        /// </summary>
        /// <param name="customData">包含自定义数据</param>
        public delegate void OnStreamCustomData(CustomDataEvent customData);

        /// <summary>
        /// 视频流回调
        /// </summary>
        /// <param name="streamUUID"></param>
        /// <param name="frame"></param>
        public delegate void OnStreamVideoData(string streamUUID, VideoFrame frame);

        /// <summary>
        /// 音频流回调。PCM 格式：双声道、16位整数
        /// </summary>
        /// <param name="streamUUID"></param>
        /// <param name="frame"></param>
        public delegate void onStreamAudioData(string streamUUID, AudioFrame frame);

        /// <summary>
        /// 日志委托
        /// </summary>
        /// <param name="message"></param>
        /// <param name="level"></param>
        public delegate void Log(string message, LogLevel level);

        /// <summary>
        /// 主播端，小程序消息通道
        /// </summary>
        /// <param name="message"></param>
        public delegate void onStreamChannelMsgData(string message);

        public OnStreamCustomData onCustomData;

        public OnStreamVideoData onVideoData;

        public onStreamAudioData onAudioData;

        public onStreamChannelMsgData onChannelMsgData;


        public Log log;

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_context_init(int thriftPort, string jobId, string logDir);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_context_init3(int thriftPort, string jobId, string logDir, int thriftListenPort, bool enableDumpCapture);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern void cpp_context_uninit();

        private delegate void CustomDataCallback(IntPtr userData, string pipelineId, CustomDataEvent e);
        private CustomDataCallback customDataCallback;

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_set_custom_data_cb(IntPtr cb, IntPtr userData);

        private delegate void VideoDataCallback2(IntPtr userData, string pipelineId, CppVideoCallbackData videoData);
        private VideoDataCallback2 videoCallback2;

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_set_video_data_cb2(IntPtr cb, IntPtr userData);

        private delegate void DumpSharedTextureCallback(IntPtr userData, IntPtr texturePtr, RawVideoFrame videoData);
        private DumpSharedTextureCallback dumpSharedTextureCallback;

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_pipeline_dump_sharedtexture(IntPtr cb, IntPtr userData, long sharedHandle, IntPtr context, IntPtr contextTexture);


        private delegate void AudioDataCallback2(IntPtr userData, string pipelineId, CppAudioCallbackData audioData);
        private AudioDataCallback2 audioCallback2;

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_set_audio_data_cb2(IntPtr cb, IntPtr userData);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_pipeline_add_raw(string streamUUID, RawVideoFrame frame, IntPtr captureStartTickMs, ref long pts);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_pipeline_add_texture(string streamUUID, IntPtr context, IntPtr texturePtr, int textureType, ref long pts, ref long fps);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_pipeline_add_texture_jce(string streamUUID, IntPtr context, IntPtr texturePtr, int textureType, ref long pts, ref long fps, IntPtr customData, uint customDataSize);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_pipeline_add_pcm(string streamUUID, PcmInfo pcm, IntPtr strData, uint strDataSize, ref long pts);

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_pipeline_add_pcm_jce(string streamUUID, PcmInfo pcm, IntPtr strData, uint strDataSize, ref long pts, IntPtr customData, uint customDataSize);

        private delegate void CppCallChannelMsgCallBack(IntPtr userData, string msg, uint len);
        private CppCallChannelMsgCallBack cppCallChannelMsgCallBack;

        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_call_channel_msg(IntPtr sendBuf, uint sendBufLen);
        [DllImport("MediaPipelineControllerSDK.dll")]
        private static extern int cpp_set_channel_msg_cb(IntPtr cb, IntPtr userData);

        private int mOutVideoFrameID = 0;
        private int mOutVideoFPS = 0;
        private GCHandle mHandleForThis;
        private List<string> mOutputStreams;
        private Dictionary<string, string> mMapFromInputStreamToOutputStream;
        private Dictionary<string, VideoCache> mMapFromInputStreamToVideoFrame;
        private Queue<Action> mAudioQueue;
        private Queue<Action> mCustomQueue;
        private Queue<Action> mChannelMsgQueue;
        private IntPtr mNativeTexturePtr;
        private VideoCache mDumpingFrame;

        /// <summary>
        /// 初始化。正式环境下使用。
        /// </summary>
        /// <returns></returns>
        public int Init(string logDir = null)
        {
            if(string.IsNullOrEmpty(logDir))
                logDir = SDKEnvironment.GetLOCAL_LOG_DIR();

            if (logDir != null)
            {
                logDir = Encoding.UTF8.GetString(Encoding.Default.GetBytes(logDir));
            }

            var port = Environment.GetEnvironmentVariable("CPP_PORT");
            if (port != null)
            {
                var jobId = Environment.GetEnvironmentVariable("CPP_JOB_ID");
                var listenPort = Environment.GetEnvironmentVariable("CPP_LISTEN_PORT");// 用于IDE调试

                // 获取推流地址
                var streamUUIDs = Environment.GetEnvironmentVariable("CPP_STREAM_UUIDS");
                if (streamUUIDs == null)
                {
                    log?.Invoke("there is no env variable named CPP_STREAM_UUIDS", LogLevel.Error);
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
                    log?.Invoke($"MediaPipelineControllerSDK thriftListenPort is null ", LogLevel.Warning);
                }
                return Init(Int32.Parse(port), thriftListenPort, jobId, logDir, streams.cpp_stream_uuids);
            }
            return Init(0, 0, "", logDir, new List<string>());
        }

        /// <summary>
        /// 初始化。建议仅本地测试时调用此重载版本。
        /// </summary>
        /// <param name="thriftPort">大于 0 的任意值。本地测试时不会用到，正式环境下从环境变量获取。</param>
        /// <param name="thriftListenPort">大于 0 的任意值。本地测试时不会用到，正式环境下从环境变量获取。</param>
        /// <param name="jobId">任意非空字符串。本地测试时不会用到，正式环境下从环境变量获取。</param>
        /// <param name="logDir">日志路径。null 时在 dll 同级目录存放日志。</param>
        /// <param name="oStreams">输出流名称</param>
        /// <returns></returns>
        public int Init(int thriftPort, int thriftListenPort, string jobId, string logDir, List<string> oStreams)
        {
            log?.Invoke($"MediaPipelineControllerSDK Init, thriftPort={thriftPort},thriftListenPort={thriftListenPort}, jobId={jobId}, logDir={logDir}", LogLevel.Info);
            mMapFromInputStreamToVideoFrame = new Dictionary<string, VideoCache>();
            mAudioQueue = new Queue<Action>();
            mCustomQueue = new Queue<Action>();
            mChannelMsgQueue = new Queue<Action>();
            mOutputStreams = oStreams;
            mMapFromInputStreamToOutputStream = new Dictionary<string, string>();
            mHandleForThis = GCHandle.Alloc(this);
            mOutVideoFrameID = 0;
            int ret = 0;
            do
            {
                // 这个在 il2cpp 下会导致崩溃
                //cpp_dump_start(null);
                //ret = cpp_context_init(thriftPort, jobId, logDir);
                ret = cpp_context_init3(thriftPort, jobId, logDir, thriftListenPort, false);
                if (ret != 0)
                {
                    log?.Invoke($"cpp_context_init error, {ret}", LogLevel.Error);
                    break;
                }
                customDataCallback = new CustomDataCallback(OnCustomDataImpl);
                ret = cpp_set_custom_data_cb(Marshal.GetFunctionPointerForDelegate(customDataCallback), GCHandle.ToIntPtr(mHandleForThis));
                if (ret != 0)
                {
                    log?.Invoke($"cpp_set_custom_data_cb error, {ret}", LogLevel.Error);
                    break;
                }
                videoCallback2 = new VideoDataCallback2(OnVideoDataImpl2);
                ret = cpp_set_video_data_cb2(Marshal.GetFunctionPointerForDelegate(videoCallback2), GCHandle.ToIntPtr(mHandleForThis));
                if (ret != 0)
                {
                    log?.Invoke($"cpp_set_video_data_cb error, {ret}", LogLevel.Error);
                    break;
                }
                audioCallback2 = new AudioDataCallback2(OnAudioDataImpl2);
                ret = cpp_set_audio_data_cb2(Marshal.GetFunctionPointerForDelegate(audioCallback2), GCHandle.ToIntPtr(mHandleForThis));
                if (ret != 0)
                {
                    log?.Invoke($"cpp_set_audio_data_cb error, {ret}", LogLevel.Error);
                    break;
                }
                cppCallChannelMsgCallBack = new CppCallChannelMsgCallBack(OnCppCallChannelMsgCallBack);
                ret = cpp_set_channel_msg_cb(Marshal.GetFunctionPointerForDelegate(cppCallChannelMsgCallBack), GCHandle.ToIntPtr(mHandleForThis));
                if (ret != 0)
                {
                    log?.Invoke($"cpp_set_channel_msg_cb error, {ret}", LogLevel.Error);
                    break;
                }

            } while (false);
            log?.Invoke($"MediaPipelineControllerSDK Init finished, {ret}", LogLevel.Info);
            return ret;
        }

        /// <summary>
        /// 逆初始化
        /// </summary>
        public void Uninit()
        {
            mAudioQueue?.Clear();
            cpp_context_uninit();
            mHandleForThis.Free();
        }

        public void Update(bool error = false)
        {
            if (!error)
            {
                lock (mMapFromInputStreamToVideoFrame)
                {
                    if (onVideoData != null && mMapFromInputStreamToVideoFrame.Count > 0)
                    {
                        long t = DateTimeOffset.Now.ToUnixTimeMilliseconds();
                        foreach (var kv in mMapFromInputStreamToVideoFrame)
                        {
                            if (!kv.Value.used)
                            {
                                if (kv.Value.sharedHandle != 0)
                                {
                                    if (dumpSharedTextureCallback == null)
                                    {
                                        dumpSharedTextureCallback = new DumpSharedTextureCallback(OnDumpSharedTextureImpl);
                                    }
                                    mDumpingFrame = kv.Value;
                                    cpp_pipeline_dump_sharedtexture(Marshal.GetFunctionPointerForDelegate(dumpSharedTextureCallback), GCHandle.ToIntPtr(mHandleForThis), kv.Value.sharedHandle, IntPtr.Zero, mNativeTexturePtr);
                                }
                                else
                                {
                                    onVideoData.Invoke(kv.Key, kv.Value.frame);
                                }
                                kv.Value.used = true;
                            }
                        }
                        t = DateTimeOffset.Now.ToUnixTimeMilliseconds() - t;
                        if (log != null && t > 10)
                        {
                            log.Invoke($"MediaPiplelineControllerSDK consume video cost {t}ms", LogLevel.Warning);
                        }
                    }
                }
            }
            
            lock (mAudioQueue)
            {
                while (mAudioQueue.Count > 0)
                {
                    mAudioQueue.Dequeue().Invoke();
                }
            }
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

        /// <summary>
        /// 指定推流 FPS
        /// </summary>
        /// <param name="fps"></param>
        public void SetOutFPS(int fps)
        {
            mOutVideoFPS = fps;
        }

        /// <summary>
        /// 随便传递一个通过 Texture2D.GetNativeTexturePtr 获取的纹理。目的是确保云加工和 Unity 用的是同一个显卡。
        /// Init 之后马上调用，主线程调用。
        /// </summary>
        /// <param name="ptr"></param>
        public void SetNativeTexturePtrForValidation(IntPtr ptr)
        {
            mNativeTexturePtr = ptr;
        }

        /// <summary>
        /// 推视频流。本地调试时，推流信息在 dll 同目录下的 ControllerSDKDebug.conf 里配置。
        /// 数据内存由 sdk 负责释放。
        /// </summary>
        /// <param name="srcStreamUUID">源流名称，即要发送的 frame 是从哪条拉流处理而来的</param>
        /// <param name="frame"></param>
        /// <returns></returns>
        public int SendVideo(string srcStreamUUID, VideoFrame frame)
        {
            var outputStream = GetOutputStreamByInputStream(srcStreamUUID);
            return SendVideoTo(outputStream, frame);
        }

        /// <summary>
        /// 推视频流。仅有一条输出流时使用。
        /// </summary>
        /// <param name="frame"></param>
        /// <returns></returns>
        public int SendVideo(VideoFrame frame)
        {
            if (mOutputStreams.Count == 0)
            {
                log?.Invoke("There is no output stream, please check your configuration.", LogLevel.Error);
                return -1;
            }
            if (mOutputStreams.Count > 1)
            {
                log?.Invoke($"There are more than one output streams in configuration but you don't specify one, use the first output stream.", LogLevel.Warning);
            }
            return SendVideoTo(mOutputStreams[0], frame);
        }

        /// <summary>
        /// 将视频流推到指定流地址。
        /// </summary>
        /// <param name="dstStreamUUID">目的地址。必须存在于 OutputStreams 列表中。</param>
        /// <param name="frame"></param>
        /// <returns></returns>
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
                log?.Invoke($"cpp_pipeline_add_raw error, {ret}", LogLevel.Error);
            }
            return ret;
        }

        // FOR TEST
        private int SendVideo2(string srcStreamUUID, RawVideoFrame frame, long pts)
        {
            var outputStream = GetOutputStreamByInputStream(srcStreamUUID);
            return cpp_pipeline_add_raw(outputStream, frame, IntPtr.Zero, ref pts);
        }

        /// <summary>
        /// 推出一个指定纹理所指代的视频帧。
        /// 只支持 DX11 纹理
        /// 必须在 UI 线程调用，保证 DX 上下文安全
        /// </summary>
        /// <param name="srcStreamUUID">源流名称，即要发送的纹理是从哪条拉流处理而来的</param>
        /// <param name="nativeTexturePtr">指代 ID3D11Resource *。可通过 Texture.GetNativeTexturePtr 获取。</param>
        /// <param name="pts"></param>
        /// <param name="customData"></param>
        /// <returns></returns>
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

        /// <summary>
        /// 推视频流。仅有一条输出流时使用。
        /// </summary>
        /// <param name="nativeTexturePtr"></param>
        /// <param name="pts"></param>
        /// <returns></returns>
        public int SendOneVideo(IntPtr nativeTexturePtr, long pts)
        {
            if (mOutputStreams.Count == 0)
            {
                log?.Invoke("There is no output stream, please check your configuration.", LogLevel.Error);
                return -1;
            }
            if (mOutputStreams.Count > 1)
            {
                log?.Invoke($"There are more than one output streams in configuration but you don't specify one, use the first output stream.", LogLevel.Warning);
            }
            return SendVideoTo(mOutputStreams[0], nativeTexturePtr, pts);
        }

        /// <summary>
        /// 将视频流推到指定流地址。
        /// </summary>
        /// <param name="dstStreamUUID">目的地址。必须存在于 OutputStreams 列表中。</param>
        /// <param name="nativeTexturePtr"></param>
        /// <param name="pts"></param>
        /// <param name="customData"></param>
        /// <returns></returns>
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

        /// <summary>
        /// 推音频流。本地调试时，推流信息在 dll 同目录下的 ControllerSDKDebug.conf 里配置。
        /// 数据内存由 sdk 负责释放。
        /// </summary>
        /// <param name="srcStreamUUID">源流名称，即要发送的 frame 是从哪条拉流处理而来的</param>
        /// <param name="frame"></param>
        /// <returns></returns>
        public int SendAudio(string srcStreamUUID, AudioFrame frame)
        {
            var outputStream = GetOutputStreamByInputStream(srcStreamUUID);
            return SendAudioTo(outputStream, frame);
        }

        /// <summary>
        /// 推音频流。仅有一条输出流时使用。
        /// </summary>
        /// <param name="frame"></param>
        /// <returns></returns>
        public int SendAudio(AudioFrame frame)
        {
            if (mOutputStreams.Count == 0)
            {
                log?.Invoke("There is no output stream, please check your configuration.", LogLevel.Error);
                return -1;
            }
            if (mOutputStreams.Count > 1)
            {
                log?.Invoke($"There are more than one output streams in configuration but you don't specify one, use the first output stream.", LogLevel.Warning);
            }
            return SendAudioTo(mOutputStreams[0], frame);
        }

        /// <summary>
        /// 将音频流推到指定流地址。
        /// </summary>
        /// <param name="dstStreamUUID">目的地址。必须存在于 OutputStreams 列表中。</param>
        /// <param name="frame"></param>
        /// <param name="customData"></param>
        /// <returns></returns>
        public int SendAudioTo(string dstStreamUUID, AudioFrame frame, byte[] customData = null)
        {
            PcmInfo pcm = new PcmInfo();
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
                log?.Invoke($"cpp_pipeline_add_pcm error, {ret}", LogLevel.Error);
            }
            return ret;
        }

        // FOR TEST
        private int SendAudio2(string streamUUID, PcmInfo pcm, IntPtr data, uint dataSize, long pts)
        {
            return cpp_pipeline_add_pcm(streamUUID, pcm, data, dataSize, ref pts);
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
        private static void OnCustomDataImpl(IntPtr userData, string pipelineId, CustomDataEvent e)
        {
            var h = GCHandle.FromIntPtr(userData);
            var sdk = (h.Target as MediaPiplelineControllerSDK);
            sdk.OnCustomData(pipelineId, e);
        }

        private void OnCustomData(string pipelineId, CustomDataEvent e)
        {
            log?.Invoke($"MediaPipelineControllerSDK CallBack OnCustomData {e.customData} LogTime = {GetLogTime()} Thread =  {Thread.CurrentThread.ManagedThreadId.ToString() }", LogLevel.Info);
            if (onCustomData != null)
            {
                lock (mCustomQueue)
                {
                    mCustomQueue.Enqueue(() =>
                    {
                        onCustomData(e);
                    });
                }
            }
        }

        private static string GetLogTime()
        {
            string str = DateTime.Now.ToString("HH:mm:ss.fff") + "";
            return str;
        }


        [MonoPInvokeCallback(typeof(VideoDataCallback2))]
        private static void OnVideoDataImpl2(IntPtr userData, string pipelineId, CppVideoCallbackData data)
        {
            var h = GCHandle.FromIntPtr(userData);
            var sdk = (h.Target as MediaPiplelineControllerSDK);
            RawVideoFrame raw;
            if (data.sharedTextureHandle == 0)
            {
                raw = (RawVideoFrame)Marshal.PtrToStructure(data.raw, typeof(RawVideoFrame));
            }
            else
            {
                raw = new RawVideoFrame();
            }
            sdk.OnVideoData(pipelineId, data.streamUUID, data.sharedTextureHandle, raw, data.pts, data.customData, data.customDataSize);
        }

        public void OnVideoData(string pipelineId, string streamUUID, long sharedHandle, RawVideoFrame frame, long pts, IntPtr customData, int customDataSize)
        {
            // SEND TEST 1
            //SendVideo2("outputstream1", frame, pts);
            // SEND TEST 2
            //         {
            //             VideoFrame dstFrame = new VideoFrame();
            //             dstFrame.width = frame.width;
            //             dstFrame.height = frame.height;
            //             dstFrame.format = frame.pixelFormat == 1 ? TextureFormat.BGRA32 : TextureFormat.RGBA32;
            //             dstFrame.data = frame.pixelData;
            //             dstFrame.dataSize = frame.lineSize * frame.height;
            //             dstFrame.pts = pts;
            //             SendVideo("outputstream1", dstFrame);
            //         }
            // 

            //Debug.Log($"MediaPipelineControllerSDK OnVideoData, pts = {pts}");
            long beginTime = DateTimeOffset.Now.ToUnixTimeMilliseconds();
            long allocTime = beginTime;
            long copyTime = 0;
            if (onVideoData != null)
            {
                var length = frame.lineSize * frame.height;
                lock (mMapFromInputStreamToVideoFrame)
                {
                    VideoCache videoCache = null;
                    if (mMapFromInputStreamToVideoFrame.ContainsKey(streamUUID))
                    {
                        videoCache = mMapFromInputStreamToVideoFrame[streamUUID];
                    }
                    else
                    {
                        videoCache = new VideoCache();
                        mMapFromInputStreamToVideoFrame[streamUUID] = videoCache;
                    }
                    if (sharedHandle == 0)
                    {
                        if (videoCache.pixelDataBuffer == null || videoCache.pixelDataBuffer.Length != length)
                        {
                            videoCache.pixelDataBuffer = new byte[length];
                            allocTime = DateTimeOffset.Now.ToUnixTimeMilliseconds();
                        }
                        Marshal.Copy(frame.pixelData, videoCache.pixelDataBuffer, 0, length);
                        copyTime = DateTimeOffset.Now.ToUnixTimeMilliseconds();
                    }
                    if (videoCache.customDataBuffer.Length < customDataSize)
                    {
                        videoCache.customDataBuffer = new byte[customDataSize];
                    }
                    if (customDataSize > 0)
                    {
                        Marshal.Copy(customData, videoCache.customDataBuffer, 0, customDataSize);
                    }
                    videoCache.sharedHandle = sharedHandle;
                    var videoFrame = videoCache.frame;
                    videoFrame.streamUUID = streamUUID;
                    videoFrame.width = frame.width;
                    videoFrame.height = frame.height;
                    videoFrame.format = frame.pixelFormat == 1 ? TextureFormat.BGRA32 : TextureFormat.RGBA32;
                    videoFrame.data = videoCache.pixelDataBuffer;
                    videoFrame.pts = pts;
                    videoFrame.customData = videoCache.customDataBuffer;
                    videoFrame.customDataSize = customDataSize;
                    videoCache.used = false;
                }
            }
            long endTime = DateTimeOffset.Now.ToUnixTimeMilliseconds();
            if (log != null && endTime - beginTime > 10)
            {
                log($"MediaPiplelineControllerSDK OnVideoData cost {endTime - beginTime}ms, alloc: {allocTime - beginTime}ms, copy: {copyTime - allocTime}ms", LogLevel.Warning);
            }
        }

        /// <summary>
        /// 同步回调（云加工同事说不好改成直接返回结果，只能回调），所以不需要加锁
        /// </summary>
        /// <param name="userData"></param>
        /// <param name="texturePtr"></param>
        /// <param name="videoData"></param>
        [MonoPInvokeCallback(typeof(DumpSharedTextureCallback))]
        private static void OnDumpSharedTextureImpl(IntPtr userData, IntPtr texturePtr, RawVideoFrame videoData)
        {
            var h = GCHandle.FromIntPtr(userData);
            var sdk = (h.Target as MediaPiplelineControllerSDK);
            var frame = sdk.mDumpingFrame.frame;
            if (texturePtr != IntPtr.Zero)
            {
                frame.nativeTex = texturePtr;
                frame.width = videoData.width;
                frame.height = videoData.height;
                frame.format = videoData.pixelFormat == 1 ? TextureFormat.BGRA32 : TextureFormat.RGBA32;
            }
            else
            {
                var length = videoData.lineSize * videoData.height;
                if (sdk.mDumpingFrame.pixelDataBuffer == null || sdk.mDumpingFrame.pixelDataBuffer.Length != length)
                {
                    sdk.mDumpingFrame.pixelDataBuffer = new byte[length];
                }
                Marshal.Copy(videoData.pixelData, sdk.mDumpingFrame.pixelDataBuffer, 0, length);
                frame.width = videoData.width;
                frame.height = videoData.height;
                frame.format = videoData.pixelFormat == 1 ? TextureFormat.BGRA32 : TextureFormat.RGBA32;
                frame.data = sdk.mDumpingFrame.pixelDataBuffer;
            }
            sdk.onVideoData.Invoke(frame.streamUUID, frame);
        }

        [MonoPInvokeCallback(typeof(AudioDataCallback2))]
        private static void OnAudioDataImpl2(IntPtr userData, string pipelineId, CppAudioCallbackData audioData)
        {
            var h = GCHandle.FromIntPtr(userData);
            var sdk = (h.Target as MediaPiplelineControllerSDK);
            PcmInfo raw;
            raw = (PcmInfo)Marshal.PtrToStructure(audioData.raw, typeof(PcmInfo));
            sdk.OnAudioData(pipelineId, audioData.streamUUID, raw, audioData.data, audioData.dataSize, audioData.pts, audioData.customData, audioData.customDataSize);
        }

        private void OnAudioData(string pipelineId, string streamUUID, PcmInfo pcm, IntPtr data, uint dataSize, long pts, IntPtr customData, int customDataSize)
        {
            // SEND TEST 1
            //SendAudio2("outputstream1", pcm, data, dataSize, pts);
            // SEND TEST 2
            //         {
            //             AudioFrame frame = new AudioFrame();
            //             frame.sampleRate = pcm.sampleRate;
            //             frame.sampleCount = pcm.sampleCount;
            //             frame.data = data;
            //             frame.dataSize = (int)dataSize;
            //             frame.pts = pts;
            //             SendAudio("outputstream1", frame);
            //         }
            // 

            //Debug.Log($"audio arrived, now = {DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}, pts = {pts}");
            if (onAudioData != null)
            {
                byte[] dataBuffer = new byte[dataSize];
                Marshal.Copy(data, dataBuffer, 0, dataBuffer.Length);
                byte[] customDataBuffer = null;
                if (customDataSize > 0)
                {
                    customDataBuffer = new byte[customDataSize];
                    Marshal.Copy(customData, customDataBuffer, 0, customDataSize);
                }
                lock (mAudioQueue)
                {
                    mAudioQueue.Enqueue(() =>
                    {
                        AudioFrame frame = new AudioFrame();
                        frame.sampleRate = pcm.sampleRate;
                        frame.sampleCount = pcm.sampleCount;
                        frame.data = dataBuffer;
                        frame.dataSize = (int)dataSize;
                        frame.pts = pts;
                        frame.customData = customDataBuffer;
                        onAudioData(streamUUID, frame);
                    });
                }
            }
        }

        [MonoPInvokeCallback(typeof(CppCallChannelMsgCallBack))]
        private static void OnCppCallChannelMsgCallBack(IntPtr userData, string msg, uint len)
        {
            var h = GCHandle.FromIntPtr(userData);
            var sdk = (h.Target as MediaPiplelineControllerSDK);
            sdk.OnCppCallChannelMsgCallBackImp(msg);
        }

        private void OnCppCallChannelMsgCallBackImp(string msg)
        {
            if (onChannelMsgData != null)
            {
                lock (mChannelMsgQueue)
                {
                    mChannelMsgQueue.Enqueue(() =>
                    {
                        onChannelMsgData(msg);
                    });
                }
            }
        }

        public void SendData(string jsonStr)
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
                log?.Invoke($"cpp_call_channel_msg error {ret}", LogLevel.Error);
                return;
            }
        }

    }
}