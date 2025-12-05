#pragma once


#include <cstdint>
#include <vector>
#include <mutex>

enum class CameraStatus{ 
   RUNNING = 2;//在线
   OFFLINE = 1;//离线
};

struct CameraInfo{
    int CameraId;
    int nvrId;
    CameraStatus status; // 1 0 -1
};

class VideoDerviceStatusInfo{
public:
    
private:
    //1.nvr
    struct NVRStatus{
        int nvrId;
        int status; // 1 0 -1
    }   
    
    std::vector<CameraInfo> cameraStatusList;
};

enum class FrameType {
    UNKNOWN,
    KEY_FRAME,      // JPEG 或 H264/H265 I-Frame
    NORMAL_FRAME
};

struct FrameData {
    std::vector<uint8_t> data;
    uint64_t timestampMs = 0;
    FrameType type = FrameType::UNKNOWN;
};

class FrameBuffer {
public:
    void setFrame(const uint8_t* buf, size_t len, FrameType type) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.assign(buf, buf + len);
        timestampMs_ = nowMs();
        type_ = type;
    }

    bool getFrame(FrameData& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (data_.empty()) return false;

        out.data = data_;
        out.timestampMs = timestampMs_;
        out.type = type_;
        return true;
    }

private:
    uint64_t nowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    std::vector<uint8_t> data_;
    uint64_t timestampMs_ = 0;
    FrameType type_ = FrameType::UNKNOWN;
    std::mutex mutex_;
};