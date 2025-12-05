#pragma once
#include ""//sdk
#include "nvr/interfaces/INVR.h"
#include "data_layer/video_data_object.h"

#include <cstring>
class Camera {//通道
public:
    Camera(const CameraInfo& info) : info_(info) {}
    CameraStatus getStatus();
    // 你需要的业务
    bool getLastKeyFrame(FrameData& out);
    void saveKeyFrame(const uint8_t* Data, size_t len);//保存 I 帧
    void onStreamHeader(uint8_t* data, size_t len);
    
private:
    void updateKeyFrame(const FrameData& frame);
    void updateStatus(CameraStatus status);
private:
    CameraInfo info_;  // 通道号、名称等
    CameraStatus status_;    // 通道的运行和 没有运行
    FrameData lastKeyFrame_; // NVR 拉到的关键帧（由 NVR SDK 写入）

    std::mutex frameMutex_;
    std::mutex statusMutex_; //运行和不运行
    friend class HKVDevice;  // 允许 NVR 层直接写入缓存（仅该类允许）
};

/**
 * @brief 海康NVR设备具体实现类
 * 核心职责：实现IDevice接口，
 */
class HKVDevice : public INVR {
public:
    HKVDevice() = default;
    ~HKVDevice() override;

    bool initSDK() override;
    bool deinitSDK() override;

    bool login(const std::string& ip, short port,
               const std::string& user, const std::string& password) override;

    bool logout() override;

    bool start(int channel,Camera* camera) override;

    bool stop(int channel,Camera* camera)override;

    bool forceIFrame(int channelId) override; // channel 是否已经 RealPlay

    bool getDeviceInfo(DeviceInfo& info) override;

    bool getSDKStatus(){return sdkInited_;}

    bool getNVRStatus(){return nvrInited_;}
private:
    void pullKeyFrameLoop();
    static void CALLBACK onHKFrameCallback(LONG lRealHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize,DWORD dwUser) ; // SDK回调函数
    Camera* findCameraByChannel(int channel);//根据这个channel来进行相关的
    bool bindCamera(Camera* camera);  // 注册 Camera

    bool isIFrame(const uint8_t* data, int len) //判断I帧    
private:
    int userId_ = -1;
    bool sdkInited_ = false;   
    bool nvrInited_ = false;
    struct ChannelContext {
        int realHandle = -1;
        Camera* camera = nullptr;
    };
    std::map<int, ChannelContext> channels_;//SDK 通道句柄 + 通道运行状态 运行状态
    std::mutex ctxMutex_;
};
