#include "nvr/branks/hikvision/HikDevice.h"
// public
bool HKVDevice::initSDK(){
    if (!NET_DVR_Init()) {
        int err = NET_DVR_GetLastError();
        throw HKVException(err, "SDK初始化失败");
    }
    // 设置连接超时（2秒，1次重试）
    NET_DVR_SetConnectTime(2000, 1);
    // 设置自动重连（10秒检测一次）
    NET_DVR_SetReconnect(10000, true);
}

bool HKVDevice::deinitSDK(){
    // 清理SDK
    NET_DVR_Cleanup();
}

bool HKVDevice::login(const std::string& ip, short port,const std::string& user, const std::string& password){
    NET_DVR_USER_LOGIN_INFO loginInfo = {0};

    // ip
    strncpy(loginInfo.sDeviceAddress, ip.c_str(), sizeof(loginInfo.sDeviceAddress) - 1);
    // 手动添加字符串结束符（确保C风格字符串正确终止）
    loginInfo.sDeviceAddress[sizeof(loginInfo.sDeviceAddress) - 1] = '\0';

    loginInfo.wPort = port;

    // 用户名
    strncpy(loginInfo.sUserName, user.c_str(), sizeof(loginInfo.sUserName) - 1);
    loginInfo.sUserName[sizeof(loginInfo.sUserName) - 1] = '\0';

    // 密码
    strncpy(loginInfo.sPassword, password.c_str(), sizeof(loginInfo.sPassword) - 1);
    loginInfo.sPassword[sizeof(loginInfo.sPassword) - 1] = '\0';
    loginInfo.bUseAsynLogin = 0;//同步登录o

    NET_DVR_DEVICEINFO_V40 deviceInfo = {0};
    userId_ = NET_DVR_Login_V40(&loginInfo,&deviceInfo);
    if (userId_ < 0) {
        int err = NET_DVR_GetLastError();
        //日志
        std::cout << "地点HKVDevice::login 错误原因:" << err << std::endl;//日志
    }
    invrInited_ = true;;

}


bool HKVDevice::logout(){
    if (nvrInited_ && userId_ >= 0) {
        int ret = NET_DVR_Logout(userId_);
        if(ret <0 ){
            int err = NET_DVR_GetLastError();
            cout << "地点HKVDevice::logout错误原因" <<  err << std::endl;//日志
        }
        userId_ = -1;
        nvrInited_ = false;
    }
}

bool HKVDevice::start(int channel, Camera* camera){
    std::lock_guard<std::mutex> lock(ctxMutex_);

    if(!sdkInited_ || !nvrInited_){
        return false;
    }
    // 1.camera 放在 运行表里面
   
    auto it = channels_.find(channel);
    if(it != channels_.end() && it->second.realHandle > 0){
        return true;
    }
    ChannelContext& ctx = channels_[channel];
    ctx.camera = camera;
    // 2.调用 海康的预览设备

    NET_DVR_PREVIEWINFO previewInfo = {0};
    previewInfo.lChannel = channel;          // 通道号（从33开始）
    previewInfo.dwStreamType = 0;            // 1-子码流（0-主码流）
    previewInfo.dwLinkMode = 0;              // 0-TCP方式
    previewInfo.hPlayWnd = 0;          // 不需要SDK解码显示，设为nullptr
    previewInfo.bBlocked = 0;                // 非阻塞模式

    streamHandle = NET_DVR_RealPlay_V40(
        userId,                // 登录句柄
        &previewInfo,                        // 预览参数
        HKVDevice::OnPreviewCallback,      // 4.注册回调函数把值给Camera
        camera                                // 传递当前实例指针
    );
    if (streamHandle < 0){
        ctx.realHandle = -1;
        ctx.camera->updateStatus(CameraStatus::OFFLINE);
        int err = NET_DVR_GetLastError();
        //日志
        std::cout << "HKVDevice::start 错误原因:" << err << std::endl;//日志
    }

    // 3. 每一个通道的 视频句柄 放在 运行表里面
    ctx.realHandle = streamHandle;
    camera->updateStatus(CameraStatus::RUNNING);
    return true;
}

bool HKVDevice::stop(int channel, Camera* camera){
    std::lock_guard<std::mutex> lock(ctxMutex_);
    if(!sdkInited_ || !nvrInited_){
        return false;
    }
    auto it = channels_.find[channel];
    if(it == channels_.end() && it->second.realHandle < 0){
        return true;
    }
    ChannelContext& ctx = channel[channel];
    ctx.camera = camera;
    ret =  NET_DVR_StopRealPlay(ctx.realHandle);
    if(ret <= 0){
        int err = NET_DVR_GetLastError();
        std::cout << "HKVDevice::start 错误原因:" << err << std::endl;//日志
        return false;
    }
    ctx.realHandle = -1;
    camera->updateStatus(CameraStatus::ONLINE);
    return true;
}

// private
void CALLBACK HKVDevice::OnPreviewCallback(LONG lRealHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize,void *dwUser){
    
    Camera* camera = static_cast<Camera*>(dwUser);//reinterpret_cast
    NET_DVR_MakeKeyFrame(userId_,info_.channel)
    if (!camera) return;
    if(dwDataType == NET_NVR_STREAMDATA){
        if(isIFrame(pBuffer,dwBufSize)){
            camera->onEncodedFrame(pBuffer, dwBufSize); //把这个帧
        }
    }
}

bool HKVDevice::isIFrame(const uint8_t* data, int len){
    if (len < 5) return false;
    // 找起始码 00 00 00 01 或 00 00 01
    int i = 0;
    while (i < len - 4) {
        if (data[i] == 0x00 && data[i+1] == 0x00 &&
           ((data[i+2] == 0x01) || (data[i+2] == 0x00 && data[i+3] == 0x01))) {
            break;
        }
        i++;
    }
    if (i >= len - 4) return false;

    // 跳过起始码
    if (data[i+2] == 0x01)
        i += 3;
    else
        i += 4;

    uint8_t nal = data[i];

    // 判断 H.264 / H.265
    // H.264: NAL = forbidden_zero(1bit) | ref_idc(2bit) | nal_type(5bit)
    uint8_t nal_type_h264 = nal & 0x1F;

    if (nal_type_h264 > 0 && nal_type_h264 < 32) {
        // H264
        return nal_type_h264 == 5;  // IDR 帧（I 帧）
    }

    // H.265: NAL = forbidden_zero(1bit) | nal_type(6bit) | layer(6bit)
    uint8_t nal_type_h265 = (nal >> 1) & 0x3F;

    // H265 IDR / CRA / BLA 都属于关键帧，最常用的是 19、20
    if (nal_type_h265 == 19 || nal_type_h265 == 20) {
        return true;
    }

    return false;
}

