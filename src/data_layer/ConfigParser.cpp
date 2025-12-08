#include "data_layer/config_parser.h"
#include <fstream>
#include <iostream>
#include <algorithm>  // 用于 std::sort

using json = nlohmann::json;

// 单例实例获取
ConfigParser& ConfigParser::getInstance()
{
    static ConfigParser instance;
    return instance;
}

// 加载配置文件
bool ConfigParser::loadFromFile(const std::string& path)
{
    if (isLoaded_) {
        return true; // 已加载，直接返回
    }
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "[ConfigParser] Failed to open file: " << path << "\n";
        return false;
    }

    json j;
    try {
        ifs >> j;
    } catch (std::exception& e) {
        std::cerr << "[ConfigParser] JSON parse error: " << e.what() << "\n";
        return false;
    }

    auto& root = j["device_config"];

    config_.version = root.value("version", "");
    config_.description = root.value("description", "");

    // 修正：NVR 直接在 device_config 下，而非 devices 下
    parseNVR(root);
    // 摄像头在 device_config.devices 下
    auto& devs = root["devices"];
    parseCameras(devs);

    CameraChangeChannelNo(config_.cameras);
    isLoaded_ = true;
    return true;
}

// 解析单个 NVR 配置（JSON 中 nvr 是数组，取第一个元素）
void ConfigParser::parseNVR(const json& j)
{
    if (!j.contains("nvr")) {
        std::cerr << "[ConfigParser] NVR config not found in JSON\n";
        return;
    }

    // JSON 中 nvr 是数组（即使只有一个），取第一个元素
    const auto& nvr_array = j["nvr"];
    if (nvr_array.empty()) {
        std::cerr << "[ConfigParser] NVR array is empty\n";
        return;
    }
    const auto& nvr_json = nvr_array[0];  // 取第一个 NVR 配置

    NVRConfig& c = config_.nvr;
    // 字符串类型解析，默认值为空字符串
    c.nvrId = std::stoi(nvr_json.value("id", "0"));
    c.brand = nvr_json.value("brand", "");  // 恢复 brank 字段名
    c.ip = nvr_json.value("ip", "");
    c.username = nvr_json.value("username", "");
    c.password = nvr_json.value("password", "");
    c.port = std::stoi(nvr_json.value("port", "0"));

    // std::cout << "[ConfigParser] NVR parsed: id=" << c.nvrId << ", ip=" << c.ip << "\n";
}

// 解析摄像头配置（字符串类型匹配）
void ConfigParser::parseCameras(const json& j)
{
    if (!j.contains("camera")) {
        std::cerr << "[ConfigParser] Cameras config not found in JSON\n";
        return;
    }

    const auto& camera_array = j["camera"];
    for (const auto& item : camera_array) {
        CameraConfig c;
        // 字符串类型解析，默认值为空字符串
        c.cameraId = std::stoi(item.value("id", "0"));
        c.nvrId = std::stoi(item.value("nvrId", ""));
        c.name = item.value("name", "");
        config_.cameras.push_back(c);

        // std::cout << "[ConfigParser] Camera parsed: id=" << c.cameraId << ", name=" << c.name << "\n";
    }
}

// 分配摄像头通道号（排序逻辑调整为字符串比较）
void ConfigParser::CameraChangeChannelNo(std::vector<CameraConfig>& cameras) {
    // 按 cameraId 字符串排序（保证分配顺序稳定）
    std::sort(cameras.begin(), cameras.end(), 
        [](const CameraConfig& a, const CameraConfig& b) {
            // 字符串排序，若需要数字排序可转换为 int 后比较
            // return std::stoi(a.cameraId) < std::stoi(b.cameraId);
            return a.cameraId < b.cameraId;
        });

    // 分配通道号，从 1 开始
    int channelNo = 1;
    for (auto& camera : cameras) {
        camera.channelNo = channelNo++;
        std::cout << "[ConfigParser] Camera " << camera.cameraId << " assigned channel: " << camera.channelNo << "\n";
    }
}