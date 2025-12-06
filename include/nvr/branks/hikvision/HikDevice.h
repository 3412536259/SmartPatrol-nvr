#pragma once
#include ""//sdk
#include "nvr/interfaces/INVR.h"
#include "data_layer/video_data_object.h"

#include <cstring>
class Camera {//通道 //解码相应的H264/H265 帧 ，保存最新的帧（RGB）,
public:
   Camera(const CameraInfo& info)
        : info_(info), status_(CameraStatus::OFFLINE), codecCtx_(nullptr),
          frameRGB_(nullptr), swsCtx_(nullptr) {
        initDecoder(info.codecId);
    }

    ~Camera() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (frameRGB_) av_frame_free(&frameRGB_);
        if (codecCtx_) avcodec_free_context(&codecCtx_);
        if (swsCtx_) sws_freeContext(swsCtx_);
    }

    CameraStatus getStatus();
    // 你需要的业务
    bool getLastKeyFrame(FrameData& out);//得到最新帧

    void onEncodedFrame(uint8_t* Data, size_t len);// 海康SDK回调来的编码帧

    
private:
    void logFFmpegError(const char* func, int err);

    bool initDecoder();

    void updateKeyFrame(FrameData lastKeyFrame);

    void updateStatus(CameraStatus status);

private:
    CameraInfo info_;
    CameraStatus status_;

    // 最新帧
    FrameData lastKeyFrame_;//这个是保存帧的
    std::mutex frameMutex_;

    // 解码器
    AVCodecContext* codecCtx_;
    AVFrame* frameRGB_;
    SwsContext* swsCtx_;

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

    // bool forceIFrame(int channelId) override; // channel 是否已经 RealPlay

    bool getDeviceInfo(DeviceInfo& info) override;

    bool getSDKStatus(){return sdkInited_;}

    bool getNVRStatus(){return nvrInited_;}
private:
    // void pullKeyFrameLoop();
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
