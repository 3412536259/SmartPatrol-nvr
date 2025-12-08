#pragma once
#include "nvr/interfaces/HCNetSDK.h"//sdk
#include "nvr/interfaces/INVR.h"
#include "data_layer/video_data_object.h"
#include <cstring>
#include <map>
#include <iostream>
#include <atomic>
#include <thread>
extern "C" {
#include <libavutil/time.h>
}
/**
 * @brief 海康NVR设备具体实现类
 * 核心职责：实现IDevice接口，
 */
class HKVDevice : public INVR {
public:
    HKVDevice() = default;

    ~HKVDevice() = default;

    bool initSDK() override;
    bool deinitSDK() override;

    bool login(const std::string& ip, short port,
               const std::string& user, const std::string& password) override;

    bool logout() override;

    bool start(int channel,Camera* camera) override;

    bool stop(int channel,Camera* camera)override;

    // bool forceIFrame(int channelId) override; // channel 是否已经 RealPlay

    // bool getDeviceInfo(DeviceInfo& info) override;

    bool getSDKStatus() override {return sdkInited_;}

    bool getNVRStatus() override {return nvrInited_;}
private:
    void pullRealPlayLoop(int channel);
    static void CALLBACK onHKFrameCallback(LONG lRealHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize,void *dwUser) ; // SDK回调函数
    Camera* findCameraByChannel(int channel);//根据这个channel来进行相关的
    bool bindCamera(Camera* camera);  // 注册 Camera

    bool isIFrame(const uint8_t* data, int len); //判断I帧    
    int getUserId();
private:
    int userId_ = -1;
    bool sdkInited_ = false;   
    bool nvrInited_ = false;
    struct ChannelContext {
        int realHandle = -1;
        Camera* camera = nullptr;
        std::atomic_bool running{false};
        std::thread pullThread;
        
    };
    std::map<int, ChannelContext> channels_;//SDK 通道句柄 + 通道运行状态 运行状态
    std::mutex ctxMutex_;
};
