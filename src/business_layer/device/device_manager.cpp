#include "device_manager.h"
#include "video_service.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

// 构造函数：初始化VideoService并启动
DeviceManager::DeviceManager() {

    // 创建VideoService实例
    videoService_ = std::make_shared<VideoService>();
       
}

// 析构函数：安全停止VideoService并释放资源
DeviceManager::~DeviceManager() {
   
}

// 获取设备状态（NVR在线状态 + 所有摄像头状态）
VideoDerviceStatusInfo DeviceManager::getStatus() {
    VideoDerviceStatusInfo status;
    if (!videoService_) {
        std::cerr << "[DeviceManager] VideoService is null, return default offline status" << std::endl;
        status.setNvrStatus(false);  // NVR离线
        return status;
    }

    // 调用VideoService获取最新状态
    bool ret = videoService_->getDeviceStatus(status);
    if (!ret) {
        std::cerr << "[DeviceManager] Failed to get device status from VideoService" << std::endl;
        status.setNvrStatus(false);
        status.clearCameraStatus();
    } else {
        std::cout << "[DeviceManager] Get status success - NVR: " << (status.getNvrStatus() ? "ONLINE" : "OFFLINE")
                  << ", Cameras: " << status.getCameraStatusList().size() << std::endl;
    }

    return status;
}

// 获取所有摄像头实时帧（解码后的YUV帧）
VideoFrames DeviceManager::getAllRealImage() {

    VideoFrames frames;
    if (!videoService_) {
        std::cerr << "[DeviceManager] VideoService is null, return empty frames" << std::endl;
        frames.clear();
        return frames;
    }

    // 调用VideoService获取所有摄像头最新关键帧
    bool ret = videoService_->getAllLastKeyFrames(frames);
    if (!ret) {
        std::cerr << "[DeviceManager] Failed to get all real images" << std::endl;
        frames.clear();
    } else {
        std::cout << "[DeviceManager] Get " << frames.getFrames().size() << " real frames successfully" << std::endl;
    }

    return frames;
}

// 获取单个摄像头实时帧（带camId和nvrId）
PreviewFrame DeviceManager::getRealImage(const std::string& camId, const std::string& nvrId) {
    PreviewFrame frame;
    // 初始化返回值
    frame.setCameraId(camId);
    frame.setNvrId(nvrId);
    if (!videoService_) {
        std::cerr << "[DeviceManager] VideoService is null, cannot get frame for cam: " << camId << std::endl;
        return frame;
    }
    // 构造预览流请求参数
    PreviewStream streamParam(camId, nvrId);
    // 调用VideoService获取单摄像头帧
    bool ret = videoService_->viewCameraPreviewStream(streamParam, frame);
    if (!ret) {
        std::cerr << "[DeviceManager] Failed to get frame for cam: " << camId << " (nvr: " << nvrId << ")" << std::endl;
        return frame;
    }
    // 验证帧数据有效性
    const FrameData& frameData = frame.getFrame();
    if (frameData.frame && frameData.width > 0 && frameData.height > 0) {
        std::cout << "[DeviceManager] Get frame success - Cam: " << camId
                  << ", Resolution: " << frameData.width << "x" << frameData.height
                  << ", Timestamp: " << frameData.lastKeyFrameTime << std::endl;
        // std::cout<< "[DeviceManager] " << frame.getIntegrity() <<std::endl;          
    } else {
        std::cerr << "[DeviceManager] Frame data is invalid for cam: " << camId << std::endl;
    }
    return frame;
}

// 获取所有历史帧（预留接口）
void DeviceManager::getAllHistoryImage() {
    std::cout << "[DeviceManager] getAllHistoryImage called (not implemented yet)" << std::endl;
    // 扩展实现：
    // 1. 从本地存储/云端拉取历史帧文件
    // 2. 调用FFmpeg解码历史帧
    // 3. 封装到VideoFrames返回
}

// 获取单个摄像头历史帧（预留接口）
void DeviceManager::getHistoryImage(const std::string& camId) {
    std::cout << "[DeviceManager] getHistoryImage called for cam: " << camId << " (not implemented yet)" << std::endl;
    // 扩展实现：
    // 1. 根据camId查询历史帧存储路径
    // 2. 解码指定时间段的历史帧
    // 3. 回调上层或保存到指定位置
}

// 摄像头操作（云台、参数调整等，预留接口）
void DeviceManager::operateCamera() {
    std::cout << "[DeviceManager] operateCamera called (not implemented yet)" << std::endl;
    // 扩展实现：
    // 1. 构造摄像头操作指令（如云台上下左右、焦距调整）
    // 2. 调用VideoService的摄像头控制接口
    // 3. 返回操作结果
}

// PLC设备操作（预留接口）
void DeviceManager::operatePlc(const std::string& deviceId, const std::string& cmd) {
    std::cout << "[DeviceManager] operatePlc - Device: " << deviceId << ", Cmd: " << cmd << std::endl;
    // 扩展实现：
    // 1. 初始化PLC管理器
    // 2. 发送指令到PLC设备
    // 3. 接收并处理PLC返回结果
    // 4. 上传结果到云端/回调上层
}

// 更新配置（重启VideoService加载新配置）
void DeviceManager::updateConfig() {
}



void DeviceManager::queryRecordFiles(std::string camId_,std::string startTime_,std::string endTime_,VideoFiles videoFiles){
    if (!videoService_) {
        std::cerr << "[DeviceManager] VideoService is null, return empty frames" << std::endl;
        videoFiles.clear();
        return;
    }
    videoService_->queryRecordFiles(camId_,startTime_,endTime_,videoFiles);
}