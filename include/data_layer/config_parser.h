#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// NVR 配置（单个，字段类型改为字符串）
struct NVRConfig {
    int nvrId;       // 改为字符串类型，匹配 JSON 的 "nvr_01"
    std::string brand;       // 恢复为 brank，匹配 JSON 字段
    std::string ip;
    std::string username;
    std::string password;
    int port;        // 端口号是字符串 "554"，改为 string
};

// 摄像头配置（字段类型改为字符串）
struct CameraConfig {
    int cameraId;    // 改为字符串类型，匹配 JSON 的 "10"/"2"
    int nvrId;       // 改为字符串类型，匹配 JSON 的 "nvr_01"
    std::string name;
    int channelNo;           // 通道号仍为 int（代码中分配）
};

// 全局配置
struct GlobalConfig {
    std::string version;
    std::string description;
    NVRConfig nvr;           // 单个 NVR 配置
    std::vector<CameraConfig> cameras;  // 摄像头列表
};

class ConfigParser {
private:
    ConfigParser() = default;  // 单例私有构造
    GlobalConfig config_;
    bool isLoaded_ = false;
    
private:
    
    void parseNVR(const nlohmann::json& j);  // j 直接是 device_config 下的 nvr 节点
    void parseCameras(const nlohmann::json& j); // j 是 device_config.devices 节点
    void CameraChangeChannelNo(std::vector<CameraConfig>& cameras);
public:   
    static ConfigParser& getInstance();
    // 获取配置的接口
    const GlobalConfig& getConfig() const { return config_; }
    bool loadFromFile(const std::string& path);  
};