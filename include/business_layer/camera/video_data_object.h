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
#include <libavutil/time.h>
}

//录像的文件信息
struct RecordFileMeta {
    std::string fileName;
    uint64_t fileSize;
    time_t startTime;
    time_t endTime;
};


//dao层对象
enum CameraStatus{ 
    ONLINE = 0,//在线
    RUNNING = 1,//运行
    OFFLINE = -1//离线
};

struct CameraInfo{//给底层的Camera用的
    std::string  cameraId;
    std::string  nvrId;
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
    uint64_t lastKeyFrameTime = 0;
};
//业务对象
class VideoFrame {
public:
    // 提供构造函数方便创建帧对象
    VideoFrame(std::string cameraId_, std::string nvrId_, FrameData frame_, bool integrity_ = false)
        : cameraId(cameraId_)
        , nvrId(nvrId_)
        , frame(frame_)
        , integrity(integrity_) {}

    // 提供必要的访问器
    const std::string& getCameraId() const noexcept { return cameraId; }
    const std::string& getNvrId() const noexcept { return nvrId; }
    const FrameData& getFrameData() const noexcept { return frame; }
    bool isIntegrity() const noexcept { return integrity; }

private:
    std::string cameraId;
    std::string nvrId;
    FrameData frame;
    bool integrity = false;
};

class PreviewStream {
public:
    PreviewStream() = default;
    PreviewStream(std::string  cameraId, std::string  nvrId)
        : cameraId_(cameraId), nvrId_(nvrId) {}

    std::string  getCameraId() const noexcept { return cameraId_; }
    std::string  getNvrId() const noexcept { return nvrId_; }

    void setCameraId(std::string  id) noexcept { cameraId_ = id; }
    void setNvrId(std::string  id) noexcept { nvrId_ = id; }

private:
    std::string cameraId_;
    std::string nvrId_;
};
class PreviewFrame {
public:
    PreviewFrame() = default;

    std::string getCameraId() const noexcept { return cameraId_; }
    std::string getNvrId() const noexcept { return nvrId_; }
    const FrameData& getFrame() const noexcept { return frame_; }
    bool getIntegrity() const noexcept{return integrity;}

    void setCameraId(std::string id) noexcept { cameraId_ = id; }
    void setNvrId(std::string id) noexcept { nvrId_ = id; }
    void setFrame(const FrameData& f) { frame_ = f; }
    void setIntegrity(bool integrity){integrity = integrity;}

private:
    std::string cameraId_;
    std::string nvrId_;
    FrameData frame_;
    bool integrity = false;
};

class CameraStatusInfo {
public:
    CameraStatusInfo() = default;

    std::string getCameraId() const noexcept { return cameraId_; }
    std::string getNvrId() const noexcept { return nvrId_; }
    CameraStatus getStatus() const noexcept { return status_; }

    void setCameraId(std::string id) noexcept { cameraId_ = id; }
    void setNvrId(std::string id) noexcept { nvrId_ = id; }
    void setStatus(CameraStatus st) noexcept { status_ = st; }

private:
    std::string cameraId_;
    std::string nvrId_;
    CameraStatus status_;
};

class VideoFrames {
public:
    VideoFrames() = default;

    bool getSuccess() const noexcept { return success; }  // 修正拼写错误 sucess -> success
    const std::vector<VideoFrame>& getFrames() const noexcept { return frames_; }
    
    void clear() { frames_.clear(); }
    void setSuccess(bool success_) noexcept { success = success_; }
    // 新增添加帧的方法，避免直接操作私有容器
    void addFrame(VideoFrame frame) { frames_.emplace_back(frame); }

private:
    std::vector<VideoFrame> frames_;
    bool success = true; 
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
        : info_(info), status_(CameraStatus::OFFLINE), codecCtx_(nullptr)
          {
            initDecoder();
        frameYUV_ = av_frame_alloc();
    }

    ~Camera() {
        std::lock_guard<std::mutex> lock(frameMutex_);
        if(frameYUV_) av_frame_free(&frameYUV_);
        if (codecCtx_) avcodec_free_context(&codecCtx_);
       
    }

    CameraStatus getStatus();
    // 你需要的业务
    bool getLastKeyFrame(FrameData& out);//得到最新帧

    void onEncodedFrame(uint8_t* Data, size_t len);// 海康SDK回调来的编码帧

    CameraInfo getCameraInfo(){return info_;}
private:
    void logFFmpegError(const char* func, int err);

    void updateKeyFrame(FrameData lastKeyFrame);

    void updateStatus(CameraStatus status);

    bool convertYUVToRGB(AVFrame* yuvFrame);

    bool initDecoder();
    
    AVCodecID detectCodec(uint8_t* data, size_t len);

private:
    CameraInfo info_;
    CameraStatus status_;
    std::mutex statusMutex_;

    // 最新帧
    FrameData lastKeyFrame_;//这个是保存帧的
    std::mutex frameMutex_;
    AVFrame* frameYUV_;  // YUV帧缓冲区（复用避免重复分配）
     // 解码器
    AVCodecContext* codecCtx_;
    std::mutex decoderMutex_; // 解码器操作锁
    bool decoderInitialized_ = false;
    friend class HKVDevice;  // 允许 NVR 层直接写入缓存（仅该类允许）
};
