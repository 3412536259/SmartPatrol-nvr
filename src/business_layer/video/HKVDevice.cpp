#include "nvr/branks/hikvision/hikDevice.h"
#include "nvr/interfaces/INVR.h"
// public

std::unique_ptr<INVR> NVRFactory::createNVR(const NVRConfig& nvrConfig){
    if(nvrConfig.brand == "Hikvision"){
            return std::make_unique<HKVDevice>();
    }else{
        //日志
        std::cout << "没有相应的NVR" << std::endl;
        return nullptr;
    } 
}

bool HKVDevice::initSDK() {
    // 1. 初始化海康SDK
    if (!NET_DVR_Init()) {
        int err = NET_DVR_GetLastError();
        // 日志输出：错误码 + 描述
        std::cerr << "[HKVDevice] SDK初始化失败，错误码：" << err << std::endl;
        // 若有自定义日志工具，可替换为：
        // LOG_ERROR("[HKVDevice] SDK初始化失败，错误码：%d", err);
        return false; // 初始化失败，返回false
    }

    // 2. 设置连接超时（2秒，1次重试）
    bool retConnect = NET_DVR_SetConnectTime(2000, 1);
    if (!retConnect) {
        int err = NET_DVR_GetLastError();
        std::cerr << "[HKVDevice] 设置连接超时失败，错误码：" << err << std::endl;
        // 可选：初始化成功但参数设置失败，是否返回false根据业务需求定
        // 此处选择继续执行，仅打印日志
    }

    // 3. 设置自动重连（10秒检测一次）
    bool retReconnect = NET_DVR_SetReconnect(10000, true);
    if (!retReconnect) {
        int err = NET_DVR_GetLastError();
        std::cerr << "[HKVDevice] 设置自动重连失败，错误码：" << err << std::endl;
        // 同上，可选返回false
    }
    sdkInited_ = true;
    // 4. 初始化成功，返回true
    std::cout << "[HKVDevice] SDK初始化成功" << std::endl;
    // LOG_INFO("[HKVDevice] SDK初始化成功");
    return true;
}

bool HKVDevice::deinitSDK() {
    // 1. 执行SDK清理
    if (!NET_DVR_Cleanup()) {
        int err = NET_DVR_GetLastError();
        // 打印错误日志，包含错误码
        std::cerr << "[HKVDevice] SDK反初始化失败，错误码：" << err << std::endl;
        // 自定义日志示例：LOG_ERROR("[HKVDevice] SDK反初始化失败，错误码：%d", err);
        return false; // 清理失败，返回false
    }

    // 2. 清理成功，打印日志并返回true
    std::cout << "[HKVDevice] SDK反初始化成功" << std::endl;
    // 自定义日志示例：LOG_INFO("[HKVDevice] SDK反初始化成功");
    return true;
}


bool HKVDevice::login(const std::string& ip, short port, const std::string& user, const std::string& password) {
    // 1. 前置检查：SDK 未初始化则直接返回失败
    if (!sdkInited_) {
        std::cerr << "[HKVDevice::login] SDK 未初始化，无法执行登录操作" << std::endl;
        return false;
    }

    // 2. 参数合法性检查
    if (ip.empty() || port <= 0 || user.empty() || password.empty()) {
        std::cerr << "[HKVDevice::login] 登录参数无效（IP/端口/用户名/密码为空）" << std::endl;
        return false;
    }

    // 3. 初始化登录信息结构体
    NET_DVR_USER_LOGIN_INFO loginInfo = {0};
    // IP 地址（确保字符串终止符）
    strncpy(loginInfo.sDeviceAddress, ip.c_str(), sizeof(loginInfo.sDeviceAddress) - 1);
    loginInfo.sDeviceAddress[sizeof(loginInfo.sDeviceAddress) - 1] = '\0';

    loginInfo.wPort = port;

    // 用户名
    strncpy(loginInfo.sUserName, user.c_str(), sizeof(loginInfo.sUserName) - 1);
    loginInfo.sUserName[sizeof(loginInfo.sUserName) - 1] = '\0';

    // 密码
    strncpy(loginInfo.sPassword, password.c_str(), sizeof(loginInfo.sPassword) - 1);
    loginInfo.sPassword[sizeof(loginInfo.sPassword) - 1] = '\0';

    loginInfo.bUseAsynLogin = 0; // 同步登录

    // 4. 设备信息结构体（输出参数）
    NET_DVR_DEVICEINFO_V40 deviceInfo = {0};

    // 5. 执行登录
    userId_ = NET_DVR_Login_V40(&loginInfo, &deviceInfo);
    if (userId_ < 0) {
        int err = NET_DVR_GetLastError();
        std::cerr << "[HKVDevice::login] 登录失败，IP：" << ip << "，错误码：" << err << std::endl;
        nvrInited_ = false; // 登录失败，标记为未初始化
        return false;
    }

    // 6. 登录成功处理
    std::cout << "[HKVDevice::login] 登录成功，用户ID：" << userId_ << "，设备IP：" << ip << std::endl;
    nvrInited_ = true; // 仅登录成功时标记为已初始化
    return true;
}


bool HKVDevice::logout() {
    // 1. 检查是否已登录
    if (!nvrInited_ || userId_ < 0) {
        std::cout << "[HKVDevice::logout] 设备未登录，无需执行注销操作" << std::endl;
        return true; // 无操作视为成功，避免误报失败
    }

    // 2. 执行注销操作
    BOOL ret = NET_DVR_Logout(userId_);
    if (ret == FALSE) { // 海康SDK：FALSE 表示失败，TRUE 表示成功
        int err = NET_DVR_GetLastError();
        std::cerr << "[HKVDevice::logout] 注销失败，用户ID：" << userId_ << "，错误码：" << err << std::endl;
        // 注销失败时不重置 userId_（可能需要重试），返回 false
        return false;
    }

    // 3. 注销成功，重置状态
    std::cout << "[HKVDevice::logout] 注销成功，用户ID：" << userId_ << std::endl;
    userId_ = -1;
    nvrInited_ = false; // 重置 NVR 初始化状态

    return true;
}

bool HKVDevice::start(int channel, Camera* camera){
    std::lock_guard<std::mutex> lock(ctxMutex_);
    std::cout << "检查摄像头在线表"<<std::endl;
    // 1.camera 放在 运行表里面
    auto it = channels_.find(channel);
    if(it != channels_.end() && it->second.realHandle > 0){
        return true;
    }
    ChannelContext& ctx = channels_[channel];
    ctx.camera = camera;
    
    ctx.running = true;
    camera->updateStatus(CameraStatus::RUNNING);
    ctx.pullThread = std::thread(&HKVDevice::pullRealPlayLoop,this,channel);
    return true;
}

bool HKVDevice::stop(int channel, Camera* camera){
    std::lock_guard<std::mutex> lock(ctxMutex_);
    if(!sdkInited_ || !nvrInited_){
        return false;
    }
    auto it = channels_.find(channel);
    if(it == channels_.end() && it->second.realHandle < 0){
        return true;
    }
    ChannelContext& ctx = channels_[channel];
    ctx.camera = camera;
    ctx.running = false ;
    if(ctx.pullThread.joinable()) ctx.pullThread.join();

    int ret =  NET_DVR_StopRealPlay(ctx.realHandle);
    if(ret <= 0){
        int err = NET_DVR_GetLastError();
        std::cout << "HKVDevice::start 错误原因:" << err << std::endl;//日志
        return false;
    }
    ctx.realHandle = -1;
    camera->updateStatus(CameraStatus::OFFLINE);
    return true;
}



// private
void HKVDevice::pullRealPlayLoop(int channel){
    
    {
        std::lock_guard<std::mutex> lock(ctxMutex_);
        if (!sdkInited_ || !nvrInited_) {
            return;
        }
    }
    std::cout << "开始进行拉流channel "<< channel << std::endl;
    NET_DVR_PREVIEWINFO previewInfo = {0};
    previewInfo.lChannel = channel;          // 通道号（从33开始）
    previewInfo.dwStreamType = 0;            // 1-子码流（0-主码流）
    previewInfo.dwLinkMode = 0;              // 0-TCP方式
    previewInfo.hPlayWnd = 0;          // 不需要SDK解码显示，设为nullptr
    previewInfo.bBlocked = 0;                // 非阻塞模式

    int streamHandle = NET_DVR_RealPlay_V40(
        userId_,                // 登录句柄
        &previewInfo,                        // 预览参数
        HKVDevice::onHKFrameCallback,      // 4.注册回调函数把值给Camera
        this                               // 传递当前实例指针
    );
    
    {
        std::lock_guard<std::mutex> lock(ctxMutex_);

        channels_[channel].realHandle = streamHandle;
        channels_[channel].camera->keyframeRequested = false;
        channels_[channel].camera->firstFrameReceived = false;
        channels_[channel].camera->firstFrameDeadline = av_gettime_relative() + 2 * 1000 * 1000; // 5 秒后
    }

    // if (streamHandle < 0){
    //     channels_[channel].realHandle = -1;
    //     channels_[channel].camera->updateStatus(CameraStatus::OFFLINE);
    //     int err = NET_DVR_GetLastError();
    //     //日志
    //     std::cout << "HKVDevice::start 错误原因:" << err << std::endl;//日志
    // }

    while(true){
         {
            std::lock_guard<std::mutex> lock(ctxMutex_);
            if (!channels_[channel].running)
                break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

}   


int HKVDevice::getUserId(){
    return userId_;
}


void CALLBACK HKVDevice::onHKFrameCallback(LONG lRealHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize,void *dwUser){
    if (dwUser == nullptr) {
        std::cerr << "onHKFrameCallback: dwUser is null!" << std::endl;
        return;
    }
    // 2. 转换为HKVDevice实例指针（注意指针访问用->）
    HKVDevice* hkVDevice = static_cast<HKVDevice*>(dwUser);
    if (hkVDevice == nullptr) {
        std::cerr << "onHKFrameCallback: invalid HKVDevice pointer!" << std::endl;
        return;
    }
    int channel = -1;
    Camera* camera = nullptr;
    {
        std::lock_guard<std::mutex> lock(hkVDevice->ctxMutex_);
        //遍历channels_
        for(const auto& pair : hkVDevice->channels_){
            if(pair.second.realHandle == lRealHandle){
                channel = pair.first;
                camera = pair.second.camera;
                break;
            }
        }
    }
    if (channel == -1) {
        std::cerr << "onHKFrameCallback: no channel found for handle " << lRealHandle << std::endl;
        return;
    }
    // NET_DVR_MakeKeyFrame(hkVDevice->getUserId(),channel);
    if (!camera) return;
    
    if(dwDataType == NET_DVR_STREAMDATA){
        if(hkVDevice->isIFrame(pBuffer,dwBufSize)){

            camera->firstFrameReceived = true;    // 只要有 I 帧就标记

            camera->onEncodedFrame(pBuffer, dwBufSize); //把这个帧
            return ;
        }
    }
    int64_t now = av_gettime_relative();
    if (!camera->firstFrameReceived              // 尚未收到 I 帧
        && !camera->keyframeRequested            // 还没调用过
        && now > camera->firstFrameDeadline) {   // 超时 5 秒
     
        NET_DVR_MakeKeyFrame(hkVDevice->getUserId(), channel);
        camera->keyframeRequested = true;  // ⭐只触发一次
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

