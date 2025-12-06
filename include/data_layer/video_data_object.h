#pragma once


#include <cstdint>
#include <vector>
#include <mutex>

enum class CameraStatus{ 
   RUNNING = 2;//正在运行
   OFFLINE = 1;//闲置 / 没有运行但是
};

struct CameraInfo{
    int CameraId;
    int nvrId;
    int channel;
    AVCodecID codecId; // H264/H265
    CameraStatus status; // 1 0 -1
};

class VideoDerviceStatusInfo{
public:
private:
    int nvrStatus; // 1 0 -1  
    std::vector<CameraInfo> cameraStatusList;
};

enum class FrameType {
    UNKNOWN,
    KEY_FRAME,      // H264/H265 I-Frame
    NORMAL_FRAME
};

struct FrameData {
    std::shared_ptr<AVFrame> decoded;  // 解码后的 RGB/YUV
    int width = 0;
    int height = 0;
    uint64_t timestampMs = 0;
};

