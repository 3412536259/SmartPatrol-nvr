#include "hikvision/hikDevice.h"
#include "INVR.h"
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
    // ========== 核心：配置项目内的OpenSSL库路径 ==========
    // 1. libcrypto（对应NET_SDK_INIT_CFG_LIBEAY_PATH）
    const char* libeay_path = "/home/ztl/workspace/SmartPatrol-nvr/lib/nvr/branks/hikvision/libcrypto.so.1.1";
    // 2. libssl（对应NET_SDK_INIT_CFG_SSLEAY_PATH，先确认同目录下是否有libssl.so.1.1）
    const char* ssleay_path = "/home/ztl/workspace/SmartPatrol-nvr/lib/nvr/branks/hikvision/libssl.so.1.1";

    // 检查库文件是否存在（可选，调试用）
    if (access(libeay_path, F_OK) == -1) {
        printf("libcrypto库不存在：%s\n", libeay_path);
        return -1;
    }
    if (access(ssleay_path, F_OK) == -1) {
        printf("libssl库不存在：%s，尝试用系统库替代\n", ssleay_path);
        ssleay_path = "/usr/lib/aarch64-linux-gnu/libssl.so.1.1"; // 系统库兜底
    }

    // ========== 设置SDK初始化参数 ==========
    // 配置OpenSSL加密库路径
    NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_LIBEAY_PATH, (void*)libeay_path);
    // 配置OpenSSL通信库路径
    NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_SSLEAY_PATH, (void*)ssleay_path);

    // （可选）设置SDK自身库加载路径（如果HCNetSDK.so在同目录）
    NET_DVR_LOCAL_SDK_PATH sdk_path = {0};
    strncpy(sdk_path.sPath, "/home/ztl/workspace/SmartPatrol-nvr/lib/nvr/branks/hikvision/", sizeof(sdk_path.sPath)-1);
    NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_SDK_PATH, &sdk_path);

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
    // std::cout << "检查摄像头在线表"<<std::endl;
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
    if(it == channels_.end() || it->second.realHandle < 0){
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
    }
    ctx.realHandle = -1;
    camera->updateStatus(CameraStatus::OFFLINE);
    return true;
}



// private
void HKVDevice::pullRealPlayLoop(int channel){
    ChannelContext* ctx = nullptr;
    {
        std::lock_guard<std::mutex> lock(ctxMutex_);
        if (!sdkInited_ || !nvrInited_) {
            return;
        }
        auto it = channels_.find(channel);
        if (it == channels_.end()) {
            std::cerr << "[HKV] Channel " << channel << " not found in context map" << std::endl;
            return;
        }
        ctx = &(it->second);


    }
    // NET_DVR_MakeKeyFrame(userId_, channel);
    // std::cout << "开始进行拉流channel "<< channel << std::endl;
    NET_DVR_PREVIEWINFO previewInfo = {0};
    previewInfo.lChannel = channel;          // 通道号（从33开始）
    previewInfo.dwStreamType = 0;            // 1-子码流（0-主码流）
    previewInfo.dwLinkMode = 0;              // 0-TCP方式
    previewInfo.hPlayWnd = 0;          // 不需要SDK解码显示，设为nullptr
    previewInfo.bBlocked = 0;                // 非阻塞模式
    previewInfo.byProtoType = 0;


    int streamHandle = NET_DVR_RealPlay_V40(
        userId_,                // 登录句柄
        &previewInfo,                        // 预览参数
        NULL,      // 4.注册回调函数把值给Camera
        NULL                               // 传递当前实例指针
    );
    {
        std::lock_guard<std::mutex> lock(ctxMutex_);
        // std::cout << "streamHandle+" << streamHandle << "channel+" << channel << std::endl;
        if (streamHandle < 0){
            channels_[channel].realHandle = -1;
            channels_[channel].camera->updateStatus(CameraStatus::OFFLINE);
            int err = NET_DVR_GetLastError();
            //日志
            std::cout << "HKVDevice::start 错误原因:" << err << std::endl;//日志
            return;
        }
        channels_[channel].realHandle = streamHandle;
    }

    int esCallbackRet = NET_DVR_SetESRealPlayCallBack(streamHandle,HKVDevice::onHKFrameCallback,ctx);
    
    while(true)
    {   
       
         {
            std::lock_guard<std::mutex> lock(ctxMutex_);
            if (!channels_[channel].running) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

}   


void CALLBACK HKVDevice::onHKFrameCallback(LONG lPreviewHandle, NET_DVR_PACKET_INFO_EX *pstruPackInfo, void *pUser)
{
    if (pUser == nullptr) {
        std::cerr << "onHKFrameCallback: dwUser is null!" << std::endl;
        return;
    }
    auto* ctx  = reinterpret_cast<HKVDevice::ChannelContext*>(pUser);
    
    if (ctx == nullptr || ctx->camera == nullptr) {
            return;
    }   
    
    if(pstruPackInfo->dwPacketType == 1){
        // std::cout << "ctx->streamHandle+" << ctx->realHandle << "ctx->camera->channel+" << ctx->camera->getCameraInfo().channel << std::endl;
        ctx->camera->onEncodedFrame(
            pstruPackInfo->pPacketBuffer,  // 完整NALU数
            pstruPackInfo->dwPacketSize   // 完整NALU长度
        );
        // dumpPacketBuffer(pstruPackInfo->pPacketBuffer,pstruPackInfo->dwPacketSize,ctx->camera->getCameraInfo().channel,pstruPackInfo->dwPacketType);
    }

}



bool HKVDevice::queryRecordFiles(int channel,time_t start,time_t end,std::vector<RecordFileMeta>& out) {
    NET_DVR_TIME s = toHikTime(start);
    NET_DVR_TIME e = toHikTime(end);

    NET_DVR_FILECOND_V40 cond = {0};
    cond.lChannel = channel;
    cond.dwFileType = 0xff;
    cond.dwIsLocked = 0xff;
    cond.struStartTime = s;
    cond.struStopTime  = e;

    LONG handle = NET_DVR_FindFile_V40(userId_, &cond);
    if (handle < 0) return false;

    NET_DVR_FINDDATA_V40 data;
    while (true) {
        int ret = NET_DVR_FindNextFile_V40(handle, &data);
        if (ret == NET_DVR_FILE_SUCCESS) {
            RecordFileMeta meta;
            meta.fileName = data.sFileName;
            meta.fileSize = data.dwFileSize;
            meta.startTime = fromHikTime(data.struStartTime);
            meta.endTime   = fromHikTime(data.struStopTime);
            out.push_back(meta);
        } else {
            break;
        }
    }

    NET_DVR_FindClose_V30(handle);
    return true;
}

bool HikNVR::downloadRecordFile(int channel,const std::string& fileName,const std::string& localPath) {
    LONG handle = NET_DVR_GetFileByName(
        userId_,
        channel,
        (char*)fileName.c_str(),
        (char*)localPath.c_str()
    );
    if (handle < 0) return false;

    NET_DVR_PlayBackControl_V40(
        handle, NET_DVR_PLAYSTART, nullptr, 0, nullptr, nullptr
    );

    while (true) {
        int pos = NET_DVR_GetDownloadPos(handle);
        if (pos == 100) break;
        if (pos < 0) {
            NET_DVR_StopGetFile(handle);
            return false;
        }
        sleep(1);
    }

    NET_DVR_StopGetFile(handle);
    return true;
}

