#pragma once


#include <cstdint>
#include <vector>
#include <mutex>
#include <string>
#include <memory>
#include <iostream>
#include <map>
extern "C" {
#include <libavutil/frame.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

//dao层对象
enum CameraStatus{ 
    ONLINE = 0,//在线
    RUNNING = 1,//运行
    OFFLINE = -1//离线
};

struct CameraInfo{//给底层的Camera用的
    int cameraId;
    int nvrId;
    int channel;
    std::string name;
    CameraStatus status; 
};

struct NVRInfo{
    std::string brank;
    std::string ip;
    std::string username;
    std::string password;
    short port;
};

struct FrameData {
    std::shared_ptr<AVFrame> frame;  // 解码后的 RGB/YUV
    int width = 0;
    int height = 0;
    uint64_t timestampMs = 0;
};
//业务对象
class PreviewStream {
public:
    PreviewStream() = default;
    PreviewStream(int cameraId, int nvrId)
        : cameraId_(cameraId), nvrId_(nvrId) {}

    int getCameraId() const noexcept { return cameraId_; }
    int getNvrId() const noexcept { return nvrId_; }

    void setCameraId(int id) noexcept { cameraId_ = id; }
    void setNvrId(int id) noexcept { nvrId_ = id; }

private:
    int cameraId_{0};
    int nvrId_{0};
};
class PreviewFrame {
public:
    PreviewFrame() = default;

    int getCameraId() const noexcept { return cameraId_; }
    int getNvrId() const noexcept { return nvrId_; }
    const FrameData& getFrame() const noexcept { return frame_; }

    void setCameraId(int id) noexcept { cameraId_ = id; }
    void setNvrId(int id) noexcept { nvrId_ = id; }
    void setFrame(const FrameData& f) { frame_ = f; }

private:
    int cameraId_{0};
    int nvrId_{0};
    FrameData frame_;
};

class CameraStatusInfo {
public:
    CameraStatusInfo() = default;

    int getCameraId() const noexcept { return cameraId_; }
    int getNvrId() const noexcept { return nvrId_; }
    CameraStatus getStatus() const noexcept { return status_; }

    void setCameraId(int id) noexcept { cameraId_ = id; }
    void setNvrId(int id) noexcept { nvrId_ = id; }
    void setStatus(CameraStatus st) noexcept { status_ = st; }

private:
    int cameraId_;
    int nvrId_;
    CameraStatus status_;
};

class VideoFrames {
public:
    VideoFrames() = default;

    const std::map<int, FrameData>& getFrames() const noexcept { return frames_; }
    void addFrame(int cameraId, const FrameData& frame) {
        frames_[cameraId] = frame;
    }
    void clear() { frames_.clear(); }

private:
    std::map<int, FrameData> frames_;
};   

class VideoDerviceStatusInfo {
public:
    bool getNvrStatus() const  { return nvrStatus_; }
    const std::vector<CameraStatusInfo>& getCameraStatusList() const noexcept {
        return cameraStatusList_;
    }

    void setNvrStatus(bool st)  { nvrStatus_ = st; }
    void addCameraStatus(const CameraStatusInfo& info) {
        cameraStatusList_.push_back(info);
    }
    void clearCameraStatus() { cameraStatusList_.clear(); }

private:
    bool nvrStatus_;
    std::vector<CameraStatusInfo> cameraStatusList_;
};


//dao层的抽象的通道对象
class Camera {//通道 //解码相应的H264/H265 帧 ，保存最新的帧（RGB）,
public:
   Camera(const CameraInfo& info)
        : info_(info), status_(CameraStatus::OFFLINE), codecCtx_(nullptr),
          frameRGB_(nullptr), swsCtx_(nullptr) {
        initDecoder();
    }

    ~Camera() {
        std::lock_guard<std::mutex> lock(frameMutex_);
        if (frameRGB_) av_frame_free(&frameRGB_);
        if (codecCtx_) avcodec_free_context(&codecCtx_);
        if (swsCtx_) sws_freeContext(swsCtx_);
    }

    CameraStatus getStatus();
    // 你需要的业务
    bool getLastKeyFrame(FrameData& out);//得到最新帧

    void onEncodedFrame(uint8_t* Data, size_t len);// 海康SDK回调来的编码帧

    CameraInfo getCameraInfo(){return info_;}
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

    bool keyframeRequested = false;
    bool firstFrameReceived = false;
    int64_t firstFrameDeadline = 0; // 微秒
};
