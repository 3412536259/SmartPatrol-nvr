#pragma once
#include "HCNetSDK.h"//sdk
#include "INVR.h"
#include "video_data_object.h"
#include <cstring>
#include <map>
#include <iostream>
#include <atomic>
#include <thread>
#include <filesystem> 
#include <unordered_map>
#include <memory>
#include <iomanip> 
#include <unistd.h>
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

    bool queryRecordFiles( int channel,time_t start,time_t end,std::vector<RecordFileMeta>& outFiles);
    
    bool downloadRecordFile(int channel,const std::string& fileName,const std::string& localPath);

    // bool forceIFrame(int channelId) override; // channel 是否已经 RealPlay

    // bool getDeviceInfo(DeviceInfo& info) override;

    bool getSDKStatus() override {return sdkInited_;}

    bool getNVRStatus() override {return nvrInited_;}
private:
    void pullRealPlayLoop(int channel);
    static void CALLBACK onHKFrameCallback(LONG lPreviewHandle, NET_DVR_PACKET_INFO_EX *pstruPackInfo, void *pUser) ; // SDK回调函数
    Camera* findCameraByChannel(int channel);//根据这个channel来进行相关的
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
