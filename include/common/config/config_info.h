#pragma once
#include <string>
#include <vector>

struct NVRConfig {
    std::string nvrId;       
    std::string brand;       
    std::string ip;
    std::string username;
    std::string password;
    int port;        
};

// 摄像头配置（字段类型改为字符串）
struct CameraConfig {
    std::string cameraId;   
    std::string nvrId;       
    std::string name;
    int channelNo;           // 通道号仍为 int（代码中分配）
};

// 全局配置
struct GlobalConfig {
    std::string version;
    std::string description;
    std::string boxId = "1";
    NVRConfig nvr;           // 单个 NVR 配置
    std::vector<CameraConfig> cameras;  // 摄像头列表
};

