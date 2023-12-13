using Huya.USDK.MediaController;
using System;
using UnityEngine;
using UnityEngine.UI;
using LitJson;
using System.IO;
using System.Security.Cryptography;
using System.Collections.Generic;
using System.Linq;

public class NewBehaviourScript : MonoBehaviour
{
    public Camera mainCamera;
    public Image imageClock;
    public Button startSceneLayoutBtn;
    public Button stopSceneLayoutBtn;
    public Button pkInviteOnBtn;
    public Button pkInviteOffBtn;
    public Button normalBusBtn;
    public Button normalAppletBusBtn;
    public Text rspText;

    private Huya.USDK.MediaController.MediaPiplelineControllerSDK mediaControllerSDK; // 媒体控制SDK
    private int reqIdIndex = 0;
    private int resWidth = 800; // 可由业务侧确定游戏投屏分辨率大小
    private int resHeight = 600;
    private float ratio = -20f;
    private float timer = 0.033f; // 定时0.033f秒，业务侧可自定义
    private Texture2D gameTexture;
    private RenderTexture renderTexture;

    void Awake()
    {
        Init();
    }

    // Start is called before the first frame update
    void Start()
    {
        startSceneLayoutBtn.onClick.AddListener(OnStartSceneLayoutReqMsg);
        stopSceneLayoutBtn.onClick.AddListener(OnStopSceneLayoutReqMsg);
        pkInviteOnBtn.onClick.AddListener(OnPkInviteOnMessageListenMsg);
        pkInviteOffBtn.onClick.AddListener(OnPkInviteOffMessageListenMsg);
        normalBusBtn.onClick.AddListener(OnGetAnchorLiveStatus);
        normalAppletBusBtn.onClick.AddListener(OnGetLiveInfo);

        // 模拟测试发送获取主播端画布消息
        InvokeRepeating("TestGetAnchorCanvasMsg", 3f, 5f);
    }

    private void Init()
    {
        mediaControllerSDK = new Huya.USDK.MediaController.MediaPiplelineControllerSDK();
        mediaControllerSDK.onStreamCustomData = OnStreamCustomData;
        mediaControllerSDK.onStreamLog = OnStreamLog;
        mediaControllerSDK.onAppletInvokeRspMsg = OnDealAppletInvokeRspMsg;
        mediaControllerSDK.onAppletInvokeListenRspMsg = OnDealAppletInvokeListenMsg;
        mediaControllerSDK.onAppletMsg = OnDealAppletMsg;
        mediaControllerSDK.onAnchorMsg = OnDealAnchorMsg;

        mediaControllerSDK.SetOutFPS(30);
        mediaControllerSDK.Init();
    }

    // Update is called once per frame
    void Update()
    {
        // 时钟旋转
        TimeSpan timeSpan = DateTime.Now.TimeOfDay;
        imageClock.rectTransform.localRotation = Quaternion.Euler(0f, 0f, ratio * (float)timeSpan.TotalSeconds + 90f);

        mediaControllerSDK.Update();

        // 控制更新发送视频画面频率
        // 注：若不需要特别定制的游戏视频画面，则可不用调用SendVideoData，由主播端自动采集游戏画面
        timer -= Time.deltaTime;
        if (timer <= 0)
        {
            //SendVideoData();
            timer = 0.033f;
        }
    }

    private void SendVideoData()
    {
        // 业务侧可根据自己的需求，自行控制视频画面的分辨率，此处暂取摄像头像素画面大小
        resWidth = mainCamera.pixelWidth;
        resHeight = mainCamera.pixelHeight;

        // TODO：游戏画面纹理是翻转的，业务侧自行添加shader，以控制上下翻转
        if (renderTexture == null)
        {
            RenderTextureDescriptor renderTextureDescriptor =
                new RenderTextureDescriptor(resWidth, resHeight, RenderTextureFormat.BGRA32);
            renderTextureDescriptor.graphicsFormat = UnityEngine.Experimental.Rendering.GraphicsFormat.R8G8B8A8_UNorm;
            renderTexture = new RenderTexture(renderTextureDescriptor);
        }

        if (gameTexture == null)
        {
            gameTexture = new Texture2D(resWidth, resHeight, TextureFormat.RGBA32, false);
        }

        mainCamera.targetTexture = renderTexture;
        mainCamera.Render();

        RenderTexture.active = renderTexture;
        //gameTexture.ReadPixels(new Rect(0, 0, resWidth, resHeight), 0, 0);
        //gameTexture.Apply();
        Graphics.ConvertTexture(renderTexture, gameTexture);
        mainCamera.targetTexture = null;
        RenderTexture.active = null;

        // 若需要多个视频画面，可以调用SendVideo且配合文档里的接口调用2.1.2 UpdateAnchorLayer
        // mediaControllerSDK.SendVideo("some stream name", gameTexture.GetNativeTexturePtr(), DateTime.Now.Millisecond);

        // 若只需要一个视频定制画面，可以调用SendOneVideo
        mediaControllerSDK.SendOneVideo(gameTexture.GetNativeTexturePtr(), DateTime.Now.Millisecond);

        // 若只需要一个视频画面且为游戏画面，则可不用调用SendVideo相关接口,
        // 主播端已可自动采集, 若业务侧需自己投屏游戏画面，则前端小程序调用hyExt.exe.launchGame时optParam的manualCastScreen为true，避免出现两个相同图层画面
    }

    public void OnStreamCustomData(CustomDataEvent customDataEvent)
    {
        // 业务侧可根据customDataEvent.customData的内容，解析出对应的数据
        // 比如主播端的画布大小，业务侧可用该画布大小创建透传图层大小也可自定义大小
        if(customDataEvent.customData == null)
        {
            return;
        }

        JsonData jsonRootData = JsonMapper.ToObject(customDataEvent.customData);
        if(!jsonRootData.ContainsKey("custom_data"))
        {
            return;
        }

        var customData = jsonRootData["custom_data"];
        if (!customData.ContainsKey("out_stream"))
        {
            return;
        }

        var outStream = customData["out_stream"];
        if (outStream.ContainsKey("width"))
        {
            resWidth = (int)outStream["width"];
        }

        if (outStream.ContainsKey("height"))
        {
            resHeight = (int)outStream["height"];
        }
    }

    public void OnStreamLog(string message, LogLevel level)
    {
        Debug.Log(message);
    }

    // 获取主播端开播状态的请求消息
    public void OnGetAnchorLiveStatus()
    {
        var eventName = "GetAnchorStatus";
        var reqId = eventName + "_" + reqIdIndex.ToString();
        JsonData messageJsonData = new JsonData();
        messageJsonData["typeName"]  ="Live";
        mediaControllerSDK.SendGeneralMsg(eventName, reqId, messageJsonData);
        reqIdIndex++;
    }

    // 通过小程序获取主播直播信息
    public void OnGetLiveInfo()
    {
        var appletApiName = "hyExt.context.getLiveInfo";
        var reqId = appletApiName + "_" + reqIdIndex.ToString();

        JsonData appletApiParamjsonData = new JsonData();
        appletApiParamjsonData.SetJsonType(JsonType.Object); // 空json对象时(空参)，此设置是必要的
        var appletApiParamStr = appletApiParamjsonData.ToJson();

        List<string> appletApiParamArray = new List<string>();
        appletApiParamArray.Add(appletApiParamStr);

        mediaControllerSDK.SendAppletInvokeReqMsg(appletApiName, appletApiParamArray, reqId);
    }

    // 模拟发送获取主播端画布信息
    public void TestGetAnchorCanvasMsg()
    {
        var eventName = "GetAnchorCanvas";
        var reqId = eventName + "_" + reqIdIndex.ToString();
        JsonData messageJsonData = new JsonData();
        messageJsonData.SetJsonType(JsonType.Object); // 空json对象时(空参)，此设置是必要的
        mediaControllerSDK.SendGeneralMsg(eventName, reqId, messageJsonData);
        reqIdIndex++;
    }

    // 发送小程序调用开启场景布局的请求消息
    public void OnStartSceneLayoutReqMsg()
    {
        var appletApiName = "hyExt.stream.startSceneLayout";
        var reqId = appletApiName + "_" + reqIdIndex.ToString();

        JsonData appletApiParamjsonData = new JsonData();
        appletApiParamjsonData["scene"] = "onePkMode";
        var appletApiParamStr = appletApiParamjsonData.ToJson();

        List<string> appletApiParamArray = new List<string>();
        appletApiParamArray.Add(appletApiParamStr);

        mediaControllerSDK.SendAppletInvokeReqMsg(appletApiName, appletApiParamArray, reqId);
    }

    // 发送小程序调用停止场景布局的请求消息
    public void OnStopSceneLayoutReqMsg()
    {
        var appletApiName = "hyExt.stream.stopSceneLayout";
        var reqId = appletApiName + "_" + reqIdIndex.ToString();
        List<string> appletApiParamArray = new List<string>();
        mediaControllerSDK.SendAppletInvokeReqMsg(appletApiName, appletApiParamArray, reqId);
    }

    // 发送小程序监听类接口的请求消息
    private void OnPkInviteOnMessageListenMsg()
    {
        // 调用小程序前端JS接口：hyExt.pk.onInviteMessage
        var appletApiName = "hyExt.pk.onInviteMessage";
        var reqId = appletApiName + "_" + reqIdIndex.ToString();

        JsonData appletApiParamjsonData = new JsonData();

        // 该appletApiCallbackId即为应答里的cbId
        var appletApiCallbackId = "HYExeCallback_" + reqIdIndex.ToString();
        appletApiParamjsonData["callback"] = appletApiCallbackId;
        var appletApiParamStr = appletApiParamjsonData.ToJson();

        List<string> appletApiParamArray = new List<string>();
        appletApiParamArray.Add(appletApiParamStr);

        var listenOnOff = "on";             
        var eventType = "InviteMessage"; // 事件类型，可选
        mediaControllerSDK.SendAppletInvokeListenMsg(appletApiName, appletApiParamArray, reqId, listenOnOff, eventType);
    }

    // 发送小程序取消监听类接口的请求消息
    private void OnPkInviteOffMessageListenMsg()
    {
        // 调用小程序前端JS接口：hyExt.pk.offInviteMessage
        var appletApiName = "hyExt.pk.offInviteMessage";
        var reqId = appletApiName + "_" + reqIdIndex.ToString();
        List<string> appletApiParamArray = new List<string>();
        var listenOnOff = "off";
        var eventType = "InviteMessage"; // 事件类型，可选
        mediaControllerSDK.SendAppletInvokeListenMsg(appletApiName, appletApiParamArray, reqId, listenOnOff, eventType);
    }


    // 小程序发来的invoke应答通告消息
    private void OnDealAppletInvokeRspMsg(string apiName, string rsp, string reqId, string err)
    {
        // TODO: 业务侧自行处理小程序invoke调用应答通告
        if(apiName == "hyExt.stream.startSceneLayout")
        {
            Debug.Log("start scene layout result rsp: " + rsp + ", reqId: " + reqId);

            if(err == "")
            {
                rspText.text = "start scene layout success, rsp msg: " + rsp;
            }
            else
            {
                rspText.text = "start scene layout failed, error: " + err;
            }
        }
        else if (apiName == "hyExt.stream.stopSceneLayout")
        {
            Debug.Log("stop scene layout result rsp: " + rsp + ", reqId: " + reqId);

            if (err == "")
            {
                rspText.text = "stop scene layout success, rsp msg: " + rsp;
            }
            else
            {
                rspText.text = "stop scene layout failed, error: " + err;
            }
        }
        else if (apiName == "hyExt.pk.onInviteMessage")
        {
            Debug.Log("pk invite listen on rsp: " + rsp + ", reqId: " + reqId);

            if (err == "")
            {
                rspText.text = "pk invite listen on success, rsp msg: " + rsp;
            }
            else
            {
                rspText.text = "pk invite listen on failed, rsp error: " + err;
            }
        }
        else if (apiName == "hyExt.pk.offInviteMessage")
        {
            Debug.Log("pk invite listen off rsp: " + rsp + ", reqId: " + reqId);

            if (err == "")
            {
                rspText.text = "pk invite listen off success, rsp msg: " + rsp;
            }
            else
            {
                rspText.text = "pk invite listen off failed, rsp error: " + err;
            }
        }
        else if(apiName == "hyExt.context.getLiveInfo")
        {
            Debug.Log("get live info rsp: " + rsp + ", reqId: " + reqId);
            rspText.text = "get live info rsp msg: " + rsp;
        }
        else // 其他invoke调用应答
        {

        }
    }

    // 小程序发来的invoke listen 监听通告消息
    private void OnDealAppletInvokeListenMsg(string apiName, string message, string cbId)
    {
        // TODO: 业务侧自行处理小程序invoke listen 监听通告
        if (apiName == "hyExt.pk.onInviteMessage")
        {
            Debug.Log("get pk invite message: " + message + ", cbId: " + cbId);
            rspText.text = "pk invite msg: " + message;

            // message转json后，取PkNotice各字段值即可
            // 参考文档：https://dev.huya.com/docs/miniapp/danmugame/open/1v1/hy-ext-pk-on-invite-message/
            JsonData messageJsonData = JsonMapper.ToObject(message);
            if (messageJsonData.ContainsKey("name") && messageJsonData.ContainsKey("message"))
            {
                // TODO: 业务侧自行处理小程序发来的pk邀请消息

            }
        }
        else if (apiName == "hyExt.pk.offInviteMessage")
        {
            Debug.Log("get pk invite off message: " + message + ", cbId: " + cbId);
            rspText.text = "pk invite msg: " + message;
        }
        else // 其他invoke监听通告
        {

        }
    }

    // 小程序前端发来的消息
    private void OnDealAppletMsg(string message)
    {
        // TODO: 业务侧处理小程序发来的其他常规消息(由业务侧自定义结构和内容)
        // 若业务侧不参与小程序前端代码开发，可忽略此消息解析实现
    }

    // 主播端发来的消息
    private void OnDealAnchorMsg(string eventName, JsonData messageJsonData)
    {
        if (eventName == "GetAnchorStatus_Callback") // 对应GetAnchorLiveStatus请求应答
        {
            if (messageJsonData.ContainsKey("state"))
            {
                var state = (string)messageJsonData["state"];
                Debug.Log("get GetAnchorStatus msg, state: " + state);

                rspText.text = "anchor state: " + state;
            }
        }
        else if (eventName == "GetAnchorCanvas_Callback") // 对应TestGetAnchorCanvasMsg请求应答
        {
            var layoutType = 0;
            var width = 0;
            var height = 0;
            if (messageJsonData.ContainsKey("layoutType"))
            {
                layoutType = (int)messageJsonData["layoutType"];
            }

            if (messageJsonData.ContainsKey("width"))
            {
                width = (int)messageJsonData["width"];
            }

            if (messageJsonData.ContainsKey("height"))
            {
                height = (int)messageJsonData["height"];
            }

            Debug.Log("get GetAnchorCanvas msg: " + layoutType + ", width: " + width + ", height: " + height);

            // TODO: 模拟反向旋转UI画面(测试用的)
            ratio = -ratio;
        }
        else // TODO: 其他消息，业务侧可根据文档说明和业务需要，自行对应添加解析其他应答
        {
            Debug.Log("get applet others channel msg: " + eventName);
        }
    }
}
