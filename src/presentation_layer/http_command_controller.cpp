#include "http_service.h"

// ===================== HTTPCommandController 实现 =====================
// 修复：构造函数初始化scheduler_（需确保JobScheduler是单例/可引用）
HTTPCommandController::HTTPCommandController(JobScheduler& scheduler)
    : scheduler_(scheduler) { // 假设JobScheduler有单例接口
}

void HTTPCommandController::onMessage(const std::string& topic, const std::string& payload) {
    try {
        nlohmann::json req_json = nlohmann::json::parse(payload);
        // 根据topic分发到不同的处理函数
        if (topic == "/device/camera") {
            handleGetRealImage(req_json);

        }else if (topic == "/sensor/data") {
            handleGetSensorData(req_json);
        } else if (topic == "/device/all/status") {
            handleGetAllDeviceStatus(req_json);
        } else if(topic == "device/camera/queryFile"){
            handleHistoryVideo(payload);
        } else if(topic == "device/camera/downloadRecordFile"){
            handDownRecordFile(payload);
        }else {
            throw std::runtime_error("Unsupported topic: " + topic);
        }
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Invalid JSON payload: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to handle message: " + std::string(e.what()));
    }
}

// 占位实现（需根据业务逻辑补充）
void HTTPCommandController::handleGetRealImage(const nlohmann::json& j) {
    // 示例：调用调度器执行获取实时图像任务
    // scheduler_.scheduleJob("get_real_image", j);
    nlohmann::json resp;
    std::string camId = j.value("cameraId",j.value("deviceId", ""));
    std::string nvrId = j.value("nvrId",j.value("nvrId", ""));
    auto task = std::make_shared<GetCameraRealImageTask>(camId,nvrId);
    scheduler_.submit(task, "http");
}

void HTTPCommandController::handleGetSensorData(const nlohmann::json& j) {
    // 获取传感器数据逻辑
}

void HTTPCommandController::handleGetAllDeviceStatus(const nlohmann::json& j) {
    // 获取所有设备状态逻辑
    nlohmann::json resp;
    auto task = std::make_shared<GetDeviceStatusTask>();
    scheduler_.submit(task, "http");
}
void HTTPCommandController::handleHistoryVideo(const nlohmann::json& j){

}

void HTTPCommandController::handDownRecordFile(const nlohmann::json& j){

}

