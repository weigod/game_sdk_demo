/*
 * Interop with MediaPipelineControllerSDK.dll
*/
using System;

namespace Huya.USDK.MediaController
{
    public struct CustomDataEvent
    {
        public ulong uid;
        public string streamUrl;
        public string streamUUID;
        public string streamUUIDS;
        public string customData;
    }

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

    public struct CppVideoCallbackData
    {
        public string streamUUID;
        public IntPtr raw; //结构体指针，对应结构体RawVideoFrame
        public long pts;
        public IntPtr customData;
        public int customDataSize;
        public long sharedTextureHandle;
    }

    public struct CppAudioCallbackData
    {
        public string streamUUID;
        public IntPtr raw; //结构体指针，对应结构体PcmInfo
        public IntPtr data;
        public uint dataSize;
        public long pts;
        public IntPtr customData;
        public int customDataSize;
    }

    public struct PcmInfo
    {
        public int sampleRate;
        public int channelLayout;
        public int sampleCount;
        public int audioFormat;
    }
}